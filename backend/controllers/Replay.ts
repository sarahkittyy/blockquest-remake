import { Request, Response } from 'express';
import * as tools from '@util/tools';

import { stringify } from 'flatted';

import { prisma } from '@db/index';
import log from '@/log';

import { IsIn, IsInt, IsNumber, IsOptional, Max, Min, validate } from 'class-validator';

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
	id: number;
	user: tools.UserStub;
	levelId: number;
	time: number;
	version: string;
	raw: string;
	createdAt: number;
	updatedAt: number;
	alt: boolean;
	levelVersion: number;
	hidden: boolean;
}

export const ScoreQueryInclude = {
	include: {
		user: true,
	},
};

export const ScoreQueryHide = (uid?: number) => ({
	OR: [
		{
			hidden: false,
		},
		...(uid != undefined
			? [
					{
						userId: uid,
					},
			  ]
			: []),
	],
});

export async function uploadParsedReplay(
	data: tools.IReplayData,
	userId: number,
	levelId?: number
): Promise<tools.UserLevelScoreRunner | undefined> {
	try {
		let score: tools.UserLevelScoreRunner | undefined = await prisma.userLevelScore.findFirst({
			where: {
				userId: userId,
				levelId: levelId ?? data.header.levelId,
			},
			...ScoreQueryInclude,
		});
		const lvl = await prisma.level.findUnique({ where: { id: levelId ?? data.header.levelId } });
		if (lvl == null) throw '(NO_FIND_LVL)';
		if (score == undefined || data.header.time < score.time || score.levelVersion != lvl.version) {
			score = await prisma.userLevelScore.create({
				data: {
					user: { connect: { id: userId } },
					level: { connect: { id: levelId ?? data.header.levelId } },
					replay: data.raw,
					time: data.header.time,
					version: data.header.version,
					alt: data.header.alt,
					levelVersion: lvl.version,
				},
				...ScoreQueryInclude,
			});
		}
		return score;
	} catch (e) {
		log.warn(`Failure to upload level score: ${stringify(e)}`);
		return undefined;
	}
}

export async function uploadReplay(
	b64replay: string,
	userId: number,
	levelId?: number
): Promise<tools.UserLevelScoreRunner | undefined> {
	const buf = Buffer.from(b64replay, 'base64');
	const data: tools.IReplayData | undefined = tools.decodeRawReplay(buf);
	if (!data) return undefined;
	return await uploadParsedReplay(data, userId, levelId);
}

// replay controller
export default class Replay {
	// upload a new replay file
	static async upload(req: Request, res: Response) {
		const token: tools.IAuthToken = res.locals.token;
		const b64replay = req.body.replay;

		const replay: tools.UserLevelScoreRunner | undefined = await uploadReplay(b64replay, token.id);
		if (replay == null) {
			return res.status(400).send({ error: 'Invalid replay base64 data.' });
		}
		return res.status(200).send({ newBest: replay.time });
	}

	// hide your own replay file
	static async hide(req: Request, res: Response) {
		const token: tools.IAuthToken = res.locals.token;
		const id = parseInt(req.params.id ?? 'nan');
		const hidden: boolean = req.params.hide === 'hide';
		if (isNaN(id)) return res.status(400).send({ error: 'Invalid id parameter.' });
		try {
			const replay = await prisma.userLevelScore.findUnique({
				where: { id },
				...ScoreQueryInclude,
			});
			if (replay == null)
				return res.status(400).send({ error: 'A replay with that ID was not found.' });
			if (replay.user.id != token.id)
				return res.status(403).send({ error: 'You do not have permission to hide this replay.' });
			await prisma.userLevelScore.update({
				where: { id },
				data: { hidden },
			});
			return res.status(200).send();
		} catch (e) {
			return res.status(500).send({ error: 'Internal server error (NO_HIDE_RPL)' });
		}
	}

	static async get(req: Request, res: Response) {
		const token: tools.IAuthToken | undefined = res.locals.token;
		const id = parseInt(req.params.id ?? 'nan');
		if (isNaN(id)) return res.status(400).send({ error: 'Invalid id parameter.' });
		try {
			const replay = await prisma.userLevelScore.findFirst({
				where: {
					id,
					...ScoreQueryHide(token?.id),
				},
				...ScoreQueryInclude,
			});
			if (replay == null)
				return res.status(400).send({ error: 'A replay with that ID was not found.' });
			return res.status(200).send({ replay: tools.toReplayResponse(replay) });
		} catch (e) {
			return res.status(500).send({ error: 'Internal server error (NO_FETCH_RPL)' });
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
			take: opts.limit,
			...(opts.cursor > 1 && {
				skip: 1,
				cursor: {
					id: opts.cursor,
				},
			}),
			where: {
				levelId,
				...ScoreQueryHide(token?.id),
			},
			orderBy: [{ levelVersion: 'desc' }, { [opts.sortBy]: opts.order }],
			...ScoreQueryInclude,
		});

		if (scores.length === 0) {
			return res.status(200).send({
				scores: [],
				cursor: -1,
			});
		}
		const lastScore = scores[scores.length - 1];

		return res.status(200).send({
			scores: scores.map((score) => tools.toReplayResponse(score)),
			cursor: lastScore?.id && scores.length >= opts.limit ? lastScore.id : -1,
		});
	}
}
