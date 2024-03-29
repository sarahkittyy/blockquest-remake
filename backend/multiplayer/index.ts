import log from '@/log';
import { MP_FLUSH_INTERVAL_MS } from '@/util/constants';
import { prisma } from '@db/index';
import { User } from '@prisma/client';
import http from 'http';
import https from 'https';
import { Server, Socket } from 'socket.io';
import { v4 as uuidv4 } from 'uuid';

export let io: Server | undefined = undefined;
// central token authority
const AUTH_TOKENS: { [token: string]: number } = {};

export function generateAccessToken(userId: number): string {
	const uuid = uuidv4();
	AUTH_TOKENS[uuid] = userId;
	return uuid;
}

interface IPlayerData {
	id?: number;
	name?: string;
	outline?: number;
	fill?: number;
}

function isValidData(data: IPlayerData): boolean {
	if (data.id == undefined) return false;
	if (data.name == undefined) return false;
	if (data.outline == undefined) return false;
	if (data.fill == undefined) return false;
	return true;
}

interface ControlVars {
	xp: number;
	yp: number;
	xv: number;
	yv: number;
	sx: number;
	sy: number;
	climbing: boolean;
	dashing: boolean;
	jumping: boolean;
	dash_dir: number;
	climbing_facing: number;
	since_wallkick_ms: number;
	time_airborne_ms: number;

	this_frame: number;
	last_frame: number;
	grounded: boolean;
	facing: number;
	against_ladder_left: boolean;
	against_ladder_right: boolean;
	can_wallkick_left: boolean;
	can_wallkick_right: boolean;
	on_ice: boolean;
	flip_gravity: boolean;
	alt_controls: boolean;
	tile_above: boolean;
}

interface IPlayerState {
	id?: number;
	controls?: ControlVars;
	anim?: string;
	updatedAt?: number;
}

function isValidState(state: IPlayerState): boolean {
	if (state.id == undefined) return false;
	if (state.controls == undefined) return false;
	if (state.updatedAt == undefined) return false;
	return true;
}

interface IMessage {
	text: string;
	authorId: number;
	createdAt: number;
}

export function emitUserData(user: User) {
	const data: IPlayerData = {
		id: user.id,
		name: user.name,
		outline: parseInt(user.outlineColor, 16),
		fill: parseInt(user.fillColor, 16),
	};
	io?.emit('data_update', [data]);
}

// player states are queued and distributed at a fixed interval
const ROOMS: { [room: string]: { [id: number]: IPlayerState } } = {};

export function playerCount(levelId: number): number {
	return io?.sockets?.adapter?.rooms?.get(`${levelId}`)?.size ?? 0;
}

// run when a user connects
async function postAuthentication(socket: Socket) {
	let data = socket.data as IPlayerData;
	let room: string | undefined = undefined;
	log.info(`user ${data.name} connected`);

	socket.onAny((event) => {
		if (event === 'state_update') return;
		log.debug(`user ${data.name} ${room ?? ''} event: ${event}`);
	});

	// when a user joins
	socket.on('join', async (levelId: number) => {
		if (socket.rooms.size > 1) {
			socket.emit('warn', 'You are already in a level!');
		} else {
			room = `${levelId}`;
			await socket.join(room);
			// tell everyone in the room that the user joined
			io.in(room).emit('joined', { ...data, room });
			const socketsInRoom = await io.in(room).fetchSockets();
			const socketsData = socketsInRoom.map((s) => s.data as IPlayerData);
			socket.emit('data_update', socketsData);
			log.info(`room ${room} update: ${JSON.stringify(socketsData)} users`);
		}
	});

	// when a user leaves
	socket.on('leave', async () => {
		if (room == undefined) return;
		// tell everyone we left
		io.in(room).emit('left', data.id);
		socket.leave(room);
		log.info(`room ${room} update: ${playerCount(parseInt(room))} users`);
		room = undefined;
	});

	// data & state
	socket.on('data_update', async (new_data: IPlayerData) => {
		if (!room) return;
		if (!isValidData(data)) return;
		new_data.id = data.id;
		data = new_data;
		socket.to(room).emit('data_update', [data]);
	});

	socket.on('state_update', async (state: IPlayerState) => {
		if (!room) return;
		if (ROOMS[room] == undefined) ROOMS[room] = {};
		if (!isValidState(state)) return;
		state.id = data.id;
		ROOMS[room][state.id] = state;
		// socket.to(room).emit('state_update', [state]);
	});

	// chat
	socket.on('chat', async (msg: string) => {
		if (!room) return;
		if (msg.length == 0) return;
		io.in(room).emit('chat', <IMessage>{
			text: `[${data.name}#${data.id}] ${msg}`,
			authorId: data.id,
			createdAt: Math.floor(new Date().getTime() / 1000),
		});
	});

	socket.on('disconnect', () => {
		if (room != undefined) {
			io.in(room).emit('left', data.id);
			socket.leave(room);
			room = undefined;
		}
		log.info(`user ${data.name} disconnected`);
	});
}

// takes in a token and givenId from a socket's auth event and returns the player data if the auth goes through
async function authenticate(
	token: string | undefined,
	givenId: number | undefined
): Promise<IPlayerData | undefined> {
	if (token == undefined) return;
	if (givenId == undefined) return;

	// check for matching token and id
	if (AUTH_TOKENS?.[token] != undefined && AUTH_TOKENS[token] === givenId) {
		delete AUTH_TOKENS[token];
		const user = await prisma.user.findUnique({
			where: {
				id: givenId,
			},
		});
		if (user == null) return;
		const data: IPlayerData = {
			id: givenId,
			name: user.name,
			outline: parseInt(user.outlineColor, 16),
			fill: parseInt(user.fillColor, 16),
		};
		return data;
	}
	return;
}

// initializes socket.io
export function configure(server: http.Server | https.Server) {
	io = new Server(server, {
		transports: ['websocket'],
		cors: {
			origin: true,
		},
	});

	io.on('connection', async (socket) => {
		socket.once('authenticate', async (ev) => {
			const data = await authenticate(ev.token, ev.id);
			if (data == undefined) {
				log.info(`could not auth ${JSON.stringify(ev)}`);
				socket.emit('unauthorized');
				socket.disconnect(true);
				return;
			} else {
				socket.data = data;
				socket.emit('authorized');
				postAuthentication(socket);
			}
		});
	});

	// state update interval
	setInterval(() => {
		for (const [roomId, room] of Object.entries(ROOMS)) {
			if (Object.values(room).length == 0) {
				continue;
			}
			io.in(roomId).emit('state_update', Object.values(room));
		}
	}, MP_FLUSH_INTERVAL_MS);
}
