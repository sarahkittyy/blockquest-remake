import { Request, Response } from 'express';
import validator from 'validator';

import * as tools from '@util/tools';

import { stringify } from 'flatted';

import { prisma } from '@db/index';
import log from '@/log';
import { UserLevelScore } from '@prisma/client';

import {
	IsBoolean,
	IsIn,
	IsInt,
	IsNumber,
	IsOptional,
	IsString,
	Max,
	Min,
	validate,
	validateOrReject,
} from 'class-validator';

const SortableFields = ['time', 'author', 'createdAt', 'updatedAt'] as const;
const SortDirections = ['asc', 'desc'] as const;

export class IReplaySearchOptions {
	@IsInt({ message: 'Malformed levelId' })
	@Min(0)
	levelId!: number;

	@IsOptional()
	@IsNumber(undefined, { message: 'Malformed cursor' })
	cursor?: number;

	@IsInt({ message: 'Malformed limit' })
	@Min(1)
	@Max(20)
	limit!: number;

	@IsIn(SortableFields)
	sortBy!: typeof SortableFields[number];

	@IsIn(SortDirections)
	order!: typeof SortDirections[number];
}

export interface IReplayResponse {
	user: string;
	levelId: number;
	time: number;
	version: string;
	replay: string;
	createdAt: number;
	updatedAt: number;
}

// replay controller
export default class Replay {
	// upload a new replay file
	static async upload(req: Request, res: Response) {
		const token: tools.IAuthToken = res.locals.token;
		const b64replay = req.body.replay;
		const buf = Buffer.from(b64replay, 'base64');
		const data: tools.IReplayData | undefined = tools.decodeRawReplay(buf);
		if (!data) return res.status(400).send({ error: 'Invalid base64 replay file.', data });

		try {
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

	// search for replays
	static async search(req: Request, res: Response) {
		const token: tools.IAuthToken | undefined = res.locals.token;
		const levelId = parseInt(req.params.levelId ?? 'nan');
		if (isNaN(levelId)) {
			return res.status(400).send({ error: 'Invalid levelId parameter.' });
		}
		let opts: IReplaySearchOptions = new IReplaySearchOptions();
		try {
			opts.cursor = req.body.cursor ?? -1;
			opts.limit = req.body.limit ?? 20;
			opts.levelId = levelId;
			opts.order = req.body.order ?? 'asc';
			opts.sortBy = req.body.sortBy ?? 'time';
			const errors = await validate(opts);
			if (errors?.length > 0) {
				return res.status(400).send({
					error: Object.entries(errors[0].constraints ?? [[0, 'Malformed Request Body']])[0][1],
				});
			}
		} catch (e) {
			return res.status(400).send({ error: 'Malformed Request Body' });
		}

		const scores = await prisma.userLevelScore.findMany({
			take: opts.limit + 1,
			...(opts.cursor > 1 && {
				skip: 1,
				cursor: {
					id: opts.cursor,
				},
			}),
			where: {
				levelId,
			},
			orderBy: {
				[opts.sortBy]: opts.order,
			},
			include: {
				user: true,
			},
		});

		const lastScore = scores?.[scores.length - 2];

		return res.status(200).send({
			scores: scores.map((score) => tools.toReplayResponse(score)),
			cursor: lastScore?.id && scores.length > opts.limit ? lastScore.id : -1,
		});
	}
}
