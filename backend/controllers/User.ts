import { Request, Response } from 'express';

import * as tools from '@util/tools';
import { prisma } from '@/db';
import { ILevelResponse, LevelQueryInclude } from './Level';
import { IReplayResponse } from './Replay';
import { stringify } from 'flatted';

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
	outlineColor: number;
	fillColor: number;
	pinned?: ILevelResponse;
}

function colorIntToHexString(color: number) {
	const hex = color.toString(16);
	return `0x${'0'.repeat(8 - hex.length)}${hex}`;
}

export default class User {
	static pinLevel = (pin: boolean) => async (req: Request, res: Response) => {
		const id: number = parseInt(req.params.id ?? 'nan');
		if (isNaN(id) && pin) return res.status(400).send({ error: 'No level ID specified' });

		const token: tools.IAuthToken = res.locals.token;

		try {
			if (pin) {
				await prisma.user.update({
					where: {
						id: token.id,
					},
					data: {
						pinned: { connect: { id } },
					},
				});
			} else {
				await prisma.user.update({
					where: {
						id: token.id,
					},
					data: {
						pinned: { disconnect: true },
					},
				});
			}
			return res.status(200).send();
		} catch (e) {
			return res
				.status(500)
				.send({ error: `Could not ${pin ? 'pin' : 'unpin'} level: ${stringify(e)}` });
		}
	};

	static async setColor(req: Request, res: Response) {
		const fillColor: number | undefined = parseInt(req.body.fill);
		if (fillColor == null) return res.status(400).send({ error: 'No fill color specified' });
		const outlineColor: number | undefined = parseInt(req.body.outline);
		if (outlineColor == null) return res.status(400).send({ error: 'No outline color specified' });

		const token: tools.IAuthToken = res.locals.token;

		const fillColorHex: string = colorIntToHexString(fillColor);
		if (fillColorHex.length != 10)
			return res.status(400).send({ error: 'Invalid fill color specified' });
		const outlineColorHex: string = colorIntToHexString(outlineColor);
		if (outlineColorHex.length != 10)
			return res.status(400).send({ error: 'Invalid outline color specified' });

		try {
			await prisma.user.update({
				where: {
					id: token.id,
				},
				data: {
					fillColor: fillColorHex,
					outlineColor: outlineColorHex,
				},
			});

			return res.status(200).send();
		} catch (e) {
			return res.status(500).send({ error: 'Could not update user (NO_UPDATE_COLOR)' });
		}
	}

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
					...LevelQueryInclude(token?.id),
				},
				scores: {
					orderBy: { createdAt: 'desc' },
					include: {
						user: true,
						level: {
							...LevelQueryInclude(token?.id),
						},
					},
				},
				pinned: {
					...LevelQueryInclude(token?.id),
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
			recentLevel: user.levels[0] ? tools.toLevelResponse(user.levels[0], token?.id) : undefined,
			recentScore: user.scores[0] ? tools.toReplayResponse(user.scores[0]) : undefined,
			recentScoreLevel: user.scores[0]
				? tools.toLevelResponse(user.scores[0].level, token?.id)
				: undefined,
			outlineColor: parseInt(user.outlineColor, 16),
			fillColor: parseInt(user.fillColor, 16),
			...(user.pinned && { pinned: tools.toLevelResponse(user.pinned, token?.id) }),
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
					...LevelQueryInclude(token?.id),
				},
				scores: {
					orderBy: { createdAt: 'desc' },
					include: {
						user: true,
						level: {
							...LevelQueryInclude(token?.id),
						},
					},
				},
				pinned: {
					...LevelQueryInclude(token?.id),
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

		console.log(user);

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
			recentLevel: user.levels[0] ? tools.toLevelResponse(user.levels[0], token?.id) : undefined,
			recentScore: user.scores[0] ? tools.toReplayResponse(user.scores[0]) : undefined,
			recentScoreLevel: user.scores[0]
				? tools.toLevelResponse(user.scores[0].level, token?.id)
				: undefined,
			outlineColor: parseInt(user.outlineColor, 16),
			fillColor: parseInt(user.fillColor, 16),
			...(user.pinned && { pinned: tools.toLevelResponse(user.pinned, token?.id) }),
		};

		return res.status(200).send(stats);
	}
}
