import { Request, Response } from 'express';
import validator from 'validator';

import * as tools from '@util/tools';

import { prisma } from '@db/index';
import { Level as LevelModel, Prisma } from '@prisma/client';
import log from '@/log';

import dayjs from 'dayjs';

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
} from 'class-validator';
import { ScoreQueryHide, ScoreQueryInclude } from './Replay';
import { stringify } from 'flatted';

const SortableFields = [
	'id',
	'downloads',
	'likes',
	'dislikes',
	'createdAt',
	'updatedAt',
	'title',
	'author',
] as const;
const SortDirections = ['asc', 'desc'] as const;

export const LevelQueryInclude = (uid?: number) => ({
	include: {
		author: true,
		votes: true,
		scores: {
			...ScoreQueryInclude,
			where: {
				...ScoreQueryHide(uid),
			},
		},
		...(uid && {
			pinnedTo: {
				where: {
					id: uid,
				},
			},
		}),
		_count: {
			select: {
				comments: true,
			},
		},
	},
});

export interface ILevelResponse {
	id: number;
	code: string;
	author: tools.UserStub;
	title: string;
	description: string;
	createdAt: number;
	updatedAt: number;
	downloads: number;
	likes: number;
	dislikes: number;
	record?: {
		user: string;
		time: number;
		version: number;
	};
	myRecord?: {
		user: string;
		time: number;
		version: number;
	};
	records: number;
	comments: number;
	myVote?: 1 | 0 | -1;
	verificationId?: number;
	version: number;
	pinned?: boolean;
}

/* options for searching through levels */
export class ISearchOptions {
	@IsOptional()
	@IsNumber(undefined, { message: 'Malformed cursor' })
	cursor?: number; // current level index to search past

	@IsInt({ message: 'Malformed limit' })
	@Min(1)
	@Max(20)
	limit!: number; // how many entries to fetch

	@IsOptional()
	@IsString()
	query?: string;

	@IsBoolean({ message: 'Malformed matchTitle boolean' })
	matchTitle!: boolean;

	@IsBoolean({ message: 'Malformed matchDescription boolean' })
	matchDescription!: boolean;

	@IsBoolean({ message: 'Malformed matchAuthor boolean' })
	matchAuthor!: boolean;

	@IsBoolean({ message: 'Malformed matchSelf boolean' })
	matchSelf!: boolean;

	@IsIn(SortableFields)
	sortBy!: typeof SortableFields[number];

	@IsIn(SortDirections)
	order!: typeof SortDirections[number];
}

/* what is returned from the search endpoint */
export interface ISearchResponse {
	levels: ILevelResponse[];
	cursor: number;
}

/* level controller */
export default class Level {
	static async vote(req: Request, res: Response) {
		const token: tools.IAuthToken = res.locals.token;
		const levelId = parseInt(req.params.id);
		const vote: 'like' | 'dislike' = req.params.vote as any;

		if (isNaN(levelId)) return res.status(400).send({ error: 'Level ID is not an int.' });

		try {
			const voteModel = await prisma.userLevelVote.upsert({
				where: {
					userId_levelId: {
						userId: token.id,
						levelId: levelId,
					},
				},
				create: {
					user: { connect: { id: token.id } },
					level: { connect: { id: levelId } },
					vote: vote === 'like' ? 1 : -1,
				},
				update: {
					vote: vote === 'like' ? 1 : -1,
				},
				include: { level: { ...LevelQueryInclude(token.id) } },
			});
			return res.status(200).send({
				level: tools.toLevelResponse(voteModel.level, token.id),
			});
		} catch (e) {
			return res.status(400).send({ error: 'Level' });
		}
	}

	static async downloadPing(req: Request, res: Response) {
		let id: string | number | undefined = req.params.id;
		if (id) {
			id = parseInt(id);
			if (!isNaN(id)) {
				log.info(`Level ID ${id} download pinged`);
				await prisma.level
					.update({
						where: {
							id,
						},
						data: {
							downloads: {
								increment: 1,
							},
						},
					})
					.catch((err) => undefined);
			}
		}

		return res.status(200).send('ok');
	}
	/**
	 * search through all levels
	 *
	 * @static
	 * @async
	 * @param {ISearchOptions} req.body
	 */
	static async search(req: Request, res: Response) {
		let opts: ISearchOptions = new ISearchOptions();
		try {
			opts.cursor = req.body.cursor ?? -1;
			opts.limit = req.body.limit ?? 10;
			opts.query = req.body.query ?? '';
			opts.matchTitle = req.body.matchTitle ?? true;
			opts.matchDescription = req.body.matchDescription ?? true;
			opts.sortBy = req.body.sortBy ?? 'id';
			opts.order = req.body.order ?? 'asc';
			opts.matchSelf = req.body.matchSelf ?? false;
			opts.matchAuthor = req.body.matchAuthor ?? false;
			const errors = await validate(opts);
			if (errors?.length > 0) {
				return res.status(400).send({
					error: Object.entries(errors[0].constraints ?? [[0, 'Malformed Request Body']])[0][1],
				});
			}
		} catch (e) {
			return res.status(400).send({ error: 'Malformed Request Body' });
		}
		// fetch keywords from query
		const keywords = opts.query?.split(' ')?.map((s) => s.trim());

		const titleKeywordOr = opts.matchTitle
			? keywords?.map((word) => ({ title: { contains: word } }))
			: [];
		const descriptionKeywordOr = opts.matchDescription
			? keywords?.map((word) => ({ description: { contains: word } }))
			: [];
		const authorKeywordOr = opts.matchAuthor
			? keywords?.map((word) => ({ author: { name: { contains: word } } }))
			: [];

		const token: tools.IAuthToken | undefined = res.locals.token;

		const levels = await prisma.level.findMany({
			take: opts.limit,
			...(opts.cursor > 1 && {
				skip: 1,
				cursor: {
					id: opts.cursor,
				},
			}),
			where: {
				OR: [
					...(titleKeywordOr ?? []),
					...(descriptionKeywordOr ?? []),
					...(authorKeywordOr ?? []),
				],
				...(token != undefined && opts.matchSelf === true && { authorId: token.id }),
			},
			orderBy: {
				[opts.sortBy]: opts.order,
			},
			...LevelQueryInclude(token?.id),
		});

		if (levels.length === 0) {
			return res.status(200).send({
				levels: [],
				cursor: -1,
			});
		}
		const lastLevel = levels[levels.length - 1];

		const ret: ISearchResponse = {
			levels: levels.map((lvl) => tools.toLevelResponse(lvl, token?.id)),
			cursor: lastLevel?.id && levels.length >= opts.limit ? lastLevel.id : -1,
		};

		return res.status(200).send(ret);
	}

	/**
	 * fetch a level by its id
	 *
	 * @static
	 * @async
	 * @param {int} req.params.id
	 */
	static async getById(req: Request, res: Response) {
		const token: tools.IAuthToken | undefined = res.locals.token;
		const id: string | undefined = req.params.id;
		if (!id) {
			return res.status(400).send({ error: 'No id specified' });
		}
		const idNumber = parseInt(id);
		if (isNaN(idNumber)) {
			return res.status(400).send({ error: `Id "${id}" is not valid` });
		}

		try {
			const level = await prisma.level.update({
				where: {
					id: idNumber,
				},
				data: {
					downloads: {
						increment: 1,
					},
				},
				...LevelQueryInclude(token?.id),
			});
			if (!level) {
				return res.status(404).send({ error: `Level id "${id}" not found` });
			}
			return res.status(200).send({
				level: tools.toLevelResponse(level),
			});
		} catch (e) {
			log.error(e);
			return res.status(500).send({
				error: 'Internal server error (NO_FETCH_LEVEL)',
			});
		}
	}

	static async getQuickplay(req: Request, res: Response) {
		try {
			const token: tools.IAuthToken | undefined = res.locals.token;
			const levelIds = await prisma.level.findMany({
				select: {
					id: true,
				},
			});
			const { id } = levelIds[Math.floor(Math.random() * levelIds.length)];
			const level = await prisma.level.findUnique({
				where: { id },
				...LevelQueryInclude(token?.id),
			});
			if (!level) {
				return res.status(500).send({ error: `Quickplay fetch failed.` });
			}
			return res.status(200).send({
				level: tools.toLevelResponse(level),
			});
		} catch (e) {
			return res.status(500).send({
				error: 'Internal server error (NO_FETCH_LEVEL)',
			});
		}
	}

	/**
	 * upload a level to the server
	 *
	 * @static
	 * @async
	 * @param {string} req.body.code
	 * @param {string} req.params.confirm - for overwriting levels
	 * @param {string} req.body.title
	 * @param {string} req.body.description
	 */
	static async upload(req: Request, res: Response) {
		const code: string | undefined = req.body.code;
		const overwrite: boolean = req.params.confirm === 'confirm';
		if (!code || !tools.isValidLevel(code)) {
			return res.status(400).send({ error: 'Invalid level code.' });
		}

		const token: tools.IAuthToken | undefined = res.locals.token;
		if (!token) {
			return res.status(401).send({ error: 'Unauthorized.' });
		}
		const title: string | undefined = req.body.title;
		if (!title || !validator.isLength(title, { min: 1, max: 49 })) {
			return res.status(400).send({ error: 'Invalid title' });
		}

		const description: string | undefined = req.body.description;
		if (description && !validator.isLength(description, { max: 256 })) {
			return res.status(400).send({ error: 'Description too long! Max 256 characters' });
		}

		const user = await prisma.user.findUnique({
			where: {
				id: token.id,
			},
			include: { levels: true },
		});
		if (!user) {
			return res.status(500).send({
				error: 'Internal server error (NO_USER_FOUND)',
			});
		}
		const lastLevel = user.levels?.[user.levels.length - 1];
		if (lastLevel) {
			const secondsSinceLastLevelPosted = dayjs().diff(lastLevel.updatedAt, 'seconds');
			if (secondsSinceLastLevelPosted < 60) {
				return res.status(429).send({
					error: `Too fast! Wait ${
						60 - secondsSinceLastLevelPosted
					} more seconds before posting again.`,
				});
			}
		}

		const verificationb64: string | undefined = req.body.verification;
		if (verificationb64 == null) {
			return res.status(400).send({ error: 'You must verify a level before posting it!' });
		}
		const verification: Buffer = Buffer.from(verificationb64, 'base64');
		const replayData: tools.IReplayData | undefined = tools.decodeRawReplay(verification);
		if (replayData == null) {
			return res.status(400).send({ error: 'Invalid level verification.' });
		}

		const existingLevel = await prisma.level.findFirst({ where: { title } });
		if (existingLevel) {
			if (user.id !== existingLevel.authorId) {
				return res
					.status(401)
					.send({ error: `A level with that title already exists (ID ${existingLevel.id})` });
			}
			if (!overwrite) {
				return res
					.status(409)
					.send({ error: `A level with that title already exists (ID ${existingLevel.id})` });
			}
			const updatedLevelPreVerify = await prisma.level.update({
				where: { id: existingLevel.id },
				data: {
					title,
					description,
					code,
					updatedAt: new Date(),
					scores: {
						create: {
							user: { connect: { id: token.id } },
							replay: replayData.raw,
							time: replayData.header.time,
							version: replayData.header.version,
							alt: replayData.header.alt,
							levelVersion: existingLevel.version + 1,
						},
					},
					version: existingLevel.version + 1,
				},
				include: {
					scores: {
						orderBy: { createdAt: 'desc' },
						take: 1,
					},
				},
			});
			const updatedLevel =
				updatedLevelPreVerify != null
					? await prisma.level.update({
							where: { id: updatedLevelPreVerify.id },
							data: {
								verification: { connect: { id: updatedLevelPreVerify.scores[0].id } },
							},
							...LevelQueryInclude(token.id),
					  })
					: null;

			if (!updatedLevel)
				return res.status(500).send({ error: 'Internal server error (NO_OVERWRITE_LEVEL)' });
			log.info(
				`Level ${updatedLevel.title}#${updatedLevel.id} updated by user ${updatedLevel.author.name} (${updatedLevel.author.email})`
			);
			return res.status(200).send({
				level: tools.toLevelResponse(updatedLevel, token.id),
			});
		}

		let newLevel = await prisma.level.create({
			data: {
				code,
				author: { connect: { id: user.id } },
				title,
				description,
				scores: {
					create: {
						user: { connect: { id: user.id } },
						replay: replayData.raw,
						time: replayData.header.time,
						version: replayData.header.version,
						alt: replayData.header.alt,
					},
				},
			},
			...LevelQueryInclude(token.id),
		});
		newLevel =
			newLevel != null
				? await prisma.level.update({
						where: { id: newLevel.id },
						data: {
							verification: { connect: { id: newLevel.scores[0].id } },
						},
						...LevelQueryInclude(token.id),
				  })
				: newLevel;

		if (!newLevel) {
			return res.status(500).send({ error: 'Internal server error (NO_POST_LEVEL)' });
		}

		log.info(
			`Level ${newLevel.title}#${newLevel.id} uploaded by user ${newLevel.author.name} (${newLevel.author.email})`
		);

		return res.status(200).send({
			level: tools.toLevelResponse(newLevel, token.id),
		});
	}
}
