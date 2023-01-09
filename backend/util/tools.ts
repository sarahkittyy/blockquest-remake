import bcrypt from 'bcrypt';
import { Request, Response, NextFunction } from 'express';
import jwt from 'jsonwebtoken';

import log from '@/log';
import { User, Level, UserLevelVote, UserLevelScore, UserLevelComment } from '@prisma/client';
import { prisma } from '@db/index';
import { ILevelResponse } from '@/controllers/Level';
import { IReplayResponse } from '@/controllers/Replay';
import { ICommentResponse } from '@/controllers/Comment';
import crypto from 'crypto';

export function isValidLevel(code: string): boolean {
	const levelRegex = /^(?:[0-9]{3}|\/){1024}/g;
	if (code.match(/^[/0-9]{1023,3073}$/) == null) return false;
	if (code.match(levelRegex) == null) return false;
	return true;
}

export interface UserLevelScoreRunner extends UserLevelScore {
	user: User;
}

export interface UserLevelCommentPoster extends UserLevelComment {
	user: User;
}

export interface LevelMetadataIncluded extends Level {
	votes: UserLevelVote[];
	author: User;
	scores: Array<UserLevelScoreRunner>;
	_count: {
		comments: number;
	};
}

export interface IAuthToken {
	username: string;
	id: number;
	tier: number;
	confirmed: boolean;
}

export async function saltAndHash(password: string): Promise<string | undefined> {
	try {
		const salt = await bcrypt.genSalt(10);
		const hash = await bcrypt.hash(password, salt);
		return hash;
	} catch (e) {
		return undefined;
	}
}

export function randomString(length: number) {
	return crypto.randomBytes(length).toString('hex').slice(0, length);
}

export function validatePassword(given: string, saved: string): boolean {
	return bcrypt.compareSync(given, saved);
}

export function generateJwt(user: User): string {
	const secret = process.env.SECRET ?? 'NOTSECRET';
	return jwt.sign(
		{
			username: user.name,
			id: user.id,
			tier: user.tier,
			confirmed: user.confirmed,
		},
		secret,
		{
			expiresIn: '48h',
		}
	);
}

export function decodeToken(token: string): IAuthToken | undefined {
	if (!token) return undefined;
	try {
		const decoded = jwt.verify(token, process.env.SECRET ?? 'NOTSECRET');
		return decoded as IAuthToken;
	} catch (e) {
		return undefined;
	}
}

export function requireAuth(tier = 0, confirmed = true) {
	return async (req: Request, res: Response, next: NextFunction) => {
		const token: IAuthToken | undefined = decodeToken(req.body?.jwt);
		if (!token) {
			return res.status(401).send({ error: 'Not logged in' });
		}
		if (!token.confirmed && confirmed) {
			return res.status(409).send({ error: 'Email not verified ' });
		}
		if (token.tier < tier) {
			return res.status(401).send({ error: 'Insufficient permissions' });
		}
		res.locals.token = token;
		next();
	};
}

export function checkAuth() {
	return async (req: Request, res: Response, next: NextFunction) => {
		const token: IAuthToken | undefined = decodeToken(req.body?.jwt);
		if (token) res.locals.token = token;
		next();
	};
}

export interface IReplayHeader {
	version: string;
	levelId: number;
	created: Date;
	user: string;
	alt: boolean;
	time: number;
}

export type IReplayInputFrame = [number, number, number, number, number, number];

export interface IReplayData {
	header: IReplayHeader;
	inputs: IReplayInputFrame[];
	raw: Buffer;
}

export function decodeRawReplay(bin: Buffer | undefined): IReplayData | undefined {
	if (!bin) return undefined;
	try {
		const version: string = [...bin.toString('ascii', 0, 12)]
			.filter((v) => v.charCodeAt(0) != 0)
			.join('');
		const levelId: number = bin.readInt32LE(12);
		const created: number = bin.readInt32LE(16);
		const user: string = [...bin.toString('ascii', 20, 20 + 59)]
			.filter((v) => v.charCodeAt(0) != 0)
			.join('');
		const alt: boolean = bin.readInt8(79) > 0;
		const time: number = bin.readFloatLE(80);
		// bits 84+ are for the input data
		// 3 bytes = 4 input states
		const len = bin.byteLength;

		const testBit = (byte: number, n: number) => byte & (1 << n);

		const inputs: IReplayInputFrame[] = [];

		for (let i = 84; i < len; i += 3) {
			const bit1: number = bin[i];
			const bit2: number = bin[i + 1];
			const bit3: number = bin[i + 2];

			const i1: IReplayInputFrame = [
				testBit(bit1, 0),
				testBit(bit1, 1),
				testBit(bit1, 2),
				testBit(bit1, 3),
				testBit(bit1, 4),
				testBit(bit1, 5),
			];
			const i2: IReplayInputFrame = [
				testBit(bit1, 6),
				testBit(bit1, 7),
				testBit(bit2, 0),
				testBit(bit2, 1),
				testBit(bit2, 2),
				testBit(bit2, 3),
			];
			const i3: IReplayInputFrame = [
				testBit(bit2, 4),
				testBit(bit2, 5),
				testBit(bit2, 6),
				testBit(bit2, 7),
				testBit(bit3, 0),
				testBit(bit3, 1),
			];
			const i4: IReplayInputFrame = [
				testBit(bit3, 2),
				testBit(bit3, 3),
				testBit(bit3, 4),
				testBit(bit3, 5),
				testBit(bit3, 6),
				testBit(bit3, 7),
			];
			inputs.push(i1, i2, i3, i4);
		}
		const inputTime = inputs.length * 0.01;
		if (inputTime > time + 0.25 || inputTime < time - 0.25) {
			return undefined;
		}
		return {
			header: {
				version,
				levelId,
				created: new Date(created * 1000),
				user,
				alt,
				time,
			},
			inputs,
			raw: bin,
		};
	} catch (e) {
		return undefined;
	}
}

export function toCommentResponse(comment: UserLevelCommentPoster): ICommentResponse {
	return {
		text: comment.comment,
		createdAt: comment.createdAt.getTime() / 1000,
		updatedAt: comment.updatedAt.getTime() / 1000,
		id: comment.id,
		levelId: comment.levelId,
		user: {
			id: comment.userId,
			name: comment.user.name,
		},
	};
}

export function toReplayResponse(replay: UserLevelScoreRunner): IReplayResponse {
	return {
		id: replay.id,
		user: replay.user.name,
		levelId: replay.levelId,
		raw: replay.replay.toString('base64'),
		time: replay.time,
		version: replay.version,
		createdAt: replay.createdAt.getTime() / 1000,
		updatedAt: replay.updatedAt.getTime() / 1000,
		alt: replay.alt,
	};
}

export function toLevelResponse(lvl: LevelMetadataIncluded, userId?: number): ILevelResponse {
	const likes = likesCount(lvl.votes);
	const dislikes = dislikesCount(lvl.votes);
	if (lvl.likes !== likes || lvl.dislikes !== dislikes) {
		prisma.level
			.update({
				where: {
					id: lvl.id,
				},
				data: {
					likes,
					dislikes,
				},
			})
			.catch(log.warn);
	}

	const record = bestScore(lvl.scores);
	const myScore: UserLevelScoreRunner | undefined =
		userId != null ? userScore(lvl.scores, userId) : undefined;
	return {
		id: lvl.id,
		code: lvl.code,
		author: lvl.author.name,
		title: lvl.title,
		description: lvl.description ?? '',
		createdAt: lvl.createdAt.getTime() / 1000,
		updatedAt: lvl.updatedAt.getTime() / 1000,
		downloads: lvl.downloads,
		likes,
		dislikes,
		...(record != null && {
			record: {
				time: record.time,
				user: record.user.name,
			},
		}),
		...(myScore != undefined && {
			myRecord: {
				time: myScore.time,
				user: myScore.user.name,
			},
		}),
		records: lvl.scores.length,
		comments: lvl._count.comments,
		myVote: userId != null ? userVote(userId, lvl.votes) : undefined,
		...(lvl.verificationId != null && {
			verificationId: lvl.verificationId,
		}),
	};
}

export function userVote(userId: number, votes: UserLevelVote[]): 1 | 0 | -1 {
	return (votes.find((vote) => vote.userId === userId)?.vote as any) ?? 0;
}

export function likesCount(votes: UserLevelVote[]) {
	return votes.reduce((sum: number, vote: UserLevelVote) => {
		return vote.vote > 0 ? sum + 1 : sum;
	}, 0);
}

export async function getUserById(id: number): Promise<User> {
	try {
		const user: User | undefined = await prisma.user.findUnique({
			where: {
				id,
			},
		});
		return user;
	} catch (e) {
		return undefined;
	}
}

export function dislikesCount(votes: UserLevelVote[]) {
	return votes.reduce((sum: number, vote: UserLevelVote) => {
		return vote.vote < 0 ? sum + 1 : sum;
	}, 0);
}

export function bestScore(scores: UserLevelScoreRunner[]): UserLevelScoreRunner | undefined {
	return scores.sort((a, b) => a.time - b.time)?.[0];
}

export function userScore(
	scores: UserLevelScoreRunner[],
	userId: number
): UserLevelScoreRunner | undefined {
	return scores.find((score) => score.userId === userId);
}

export function rating(votes: UserLevelVote[]) {
	return votes.reduce((sum: number, vote: UserLevelVote) => sum + vote.vote, 0);
}
