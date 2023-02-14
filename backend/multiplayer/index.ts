import log from '@/log';
import { prisma } from '@db/index';
import http from 'http';
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
	id: number;
	name: string;
	outline: number;
	fill: number;
}

interface IPlayerState {
	id: number;
	xp: number;
	yp: number;
	xv: number;
	yv: number;
	updatedAt: number;
}

interface IPlayer {
	data: IPlayerData;
	state: IPlayerState;
}

interface IMessage {
	text: string;
	authorId: number;
	createdAt: number;
}

async function getRoomState(io: Server, room: string): Promise<IPlayer[]> {
	const sockets = await io.in(room).fetchSockets();
	return [];
}

// run when a user connects
async function postAuthentication(io: Server, socket: Socket) {
	const data = socket.data as IPlayerData;
	log.info(`user ${data.name} connected`);
	log.info(data.outline);

	// when a user joins
	socket.on('join', async (levelId: number) => {
		if (socket.rooms.size > 1) {
			socket.emit('warn', 'You are already in a level!');
		} else {
			const room = `${levelId}`;
			await socket.join(room);
			// tell everyone in the room that the user joined
			io.in(room).emit('joined', { ...data, room });
		}
	});

	// when a user leaves
	socket.on('leave', async () => {
		// go through all rooms that they're in (for safety, shouldn't be in more than 1)
		socket.rooms.forEach((room) => {
			// exclude the user's self room
			if (room !== socket.id) {
				// tell everyone we left
				io.in(room).emit('left', data.id);
				socket.leave(room);
			}
		});
	});

	socket.on('chat', async (msg: string) => {
		io.in([...socket.rooms]).emit('chat', <IMessage>{
			text: msg,
			authorId: data.id,
			createdAt: Math.floor(new Date().getTime() / 1000),
		});
	});

	socket.on('disconnect', () => {
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
export function configure(server: http.Server) {
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
				postAuthentication(io, socket);
			}
		});
	});
}
