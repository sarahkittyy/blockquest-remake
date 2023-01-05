import { Request, Response } from 'express';

import * as tools from '@util/tools';
import { prisma } from '@/db';
import { ILevelResponse, LevelQueryInclude } from './Level';
import { IReplayResponse } from './Replay';

export interface IUserStats {
	id: number;
	username: string;
	createdAt: number;
	tier: number;
	count: {
		levels: number;
		votes: number;
		comments: number;
		scores: number;
		records: number;
	};
	recentLevel?: ILevelResponse;
	recentScore?: IReplayResponse;
	recentScoreLevel?: ILevelResponse;
}

export default class User {
	static async getByName(req: Request, res: Response) {
		const token: tools.IAuthToken | undefined = res.locals.token;

		const name: string | undefined = req.params.name;
		if (name == null) return res.status(400).send({ error: 'No username specified' });

		const user = await prisma.user.findFirst({
			where: {
				name,
			},
			include: {
				levels: {
					orderBy: { createdAt: 'desc' },
					take: 1,
					...LevelQueryInclude,
				},
				scores: {
					orderBy: { createdAt: 'desc' },
					include: {
						user: true,
						level: {
							...LevelQueryInclude,
						},
					},
				},
				_count: {
					select: {
						comments: true,
						levels: true,
						votes: true,
					},
				},
			},
		});

		if (!user) return res.status(404).send({ error: 'User not found' });

		let records = 0;
		const scoreLevels = await prisma.level.findMany({
			where: {
				id: {
					in: user.scores.map((score) => score.levelId),
				},
			},
			include: {
				scores: {
					orderBy: {
						time: 'asc',
					},
					take: 1,
				},
			},
		});
		if (scoreLevels != null) {
			records = scoreLevels.filter((level) => level.scores[0].userId === user.id).length;
		}

		const stats: IUserStats = {
			id: user.id,
			createdAt: user.createdAt.getTime() / 1000,
			tier: user.tier,
			username: user.name,
			count: {
				levels: user._count.levels,
				comments: user._count.comments,
				scores: user.scores.length,
				votes: user._count.votes,
				records,
			},
			recentLevel: user.levels[0] ? tools.toLevelResponse(user.levels[0]) : undefined,
			recentScore: user.scores[0] ? tools.toReplayResponse(user.scores[0]) : undefined,
			recentScoreLevel: user.scores[0] ? tools.toLevelResponse(user.scores[0].level) : undefined,
		};

		return res.status(200).send(stats);
	}

	static async get(req: Request, res: Response) {
		const token: tools.IAuthToken | undefined = res.locals.token;

		const id = parseInt(req.params.id ?? 'nan');
		if (isNaN(id)) return res.status(400).send({ error: 'Invalid user ID' });

		const user = await prisma.user.findUnique({
			where: {
				id,
			},
			include: {
				levels: {
					orderBy: { createdAt: 'desc' },
					take: 1,
					...LevelQueryInclude,
				},
				scores: {
					orderBy: { createdAt: 'desc' },
					include: {
						user: true,
						level: {
							...LevelQueryInclude,
						},
					},
				},
				_count: {
					select: {
						comments: true,
						levels: true,
						votes: true,
					},
				},
			},
		});

		if (!user) return res.status(404).send({ error: 'User not found' });

		let records = 0;
		const scoreLevels = await prisma.level.findMany({
			where: {
				id: {
					in: user.scores.map((score) => score.levelId),
				},
			},
			include: {
				scores: {
					orderBy: {
						time: 'asc',
					},
					take: 1,
				},
			},
		});
		if (scoreLevels != null) {
			records = scoreLevels.filter((level) => level.scores[0].userId === user.id).length;
		}

		const stats: IUserStats = {
			id: user.id,
			createdAt: user.createdAt.getTime() / 1000,
			tier: user.tier,
			username: user.name,
			count: {
				levels: user._count.levels,
				comments: user._count.comments,
				scores: user.scores.length,
				votes: user._count.votes,
				records,
			},
			recentLevel: user.levels[0] ? tools.toLevelResponse(user.levels[0]) : undefined,
			recentScore: user.scores[0] ? tools.toReplayResponse(user.scores[0]) : undefined,
			recentScoreLevel: user.scores[0] ? tools.toLevelResponse(user.scores[0].level) : undefined,
		};

		return res.status(200).send(stats);
	}
}
