import { Request, Response } from 'express';
import validator from 'validator';

import * as tools from '@util/tools';

import { stringify } from 'flatted';

import { prisma } from '@db/index';
import log from '@/log';
import { UserLevelScore } from '@prisma/client';

// replay controller
export default class Replay {
	static async upload(req: Request, res: Response) {
		const token: tools.IAuthToken = res.locals.token;
		const b64replay = req.body.replay;
		const buf = Buffer.from(b64replay, 'base64');
		const data: tools.IReplayData | undefined = tools.decodeRawReplay(buf);
		if (!data) return res.status(400).send({ error: 'Invalid base64 replay file.', data });

		try {
			//const score = await prisma.$transaction([
			//prisma.userLevelScore.upsert({
			//where: {
			//id_userId_levelId:
			//},
			//create: {
			//user: { connect: { id: token.id } },
			//level: { connect: { id: data.header.levelId } },
			//time: data.header.time,
			//version: data.header.version,
			//replay: data.raw,
			//},
			//update: {
			//user: { connect: { id: token.id } },
			//level: { connect: { id: data.header.levelId } },
			//time: data.header.time,
			//version: data.header.version,
			//replay: data.raw,
			//},
			//}),
			//]);
			let score: UserLevelScore | undefined = await prisma.userLevelScore.findFirst({
				where: {
					userId: token.id,
					levelId: data.header.levelId,
				},
			});
			if (score == undefined) {
				score = await prisma.userLevelScore.create({
					data: {
						user: { connect: { id: token.id } },
						level: { connect: { id: data.header.levelId } },
						replay: buf,
						time: data.header.time,
						version: data.header.version,
					},
				});
			} else if (data.header.time <= score.time) {
				score = await prisma.userLevelScore.update({
					where: {
						userId_levelId: { userId: token.id, levelId: data.header.levelId },
					},
					data: {
						replay: buf,
						time: data.header.time,
						version: data.header.version,
					},
				});
			} else {
			}
			return res.status(200).send({ newBest: score.time });
		} catch (e) {
			log.warn(`Failure to upload level score: ${stringify(e)}`);
			return res
				.status(400)
				.send({ error: `Score failed to send ${e?.code ?? '(UNKNOWN_ERROR)'}` });
		}
	}
}
