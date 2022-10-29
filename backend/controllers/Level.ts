import { Request, Response } from 'express';
import validator from 'validator';

import * as tools from '@util/tools';

import { prisma } from '@db/index';
import { Level as LevelModel, Prisma } from '@prisma/client';
import log from '@/log';

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

const SortableFields = ['id', 'createdAt', 'updatedAt', 'name'] as const;
const SortDirections = ['asc', 'desc'] as const;

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

	@IsIn(SortableFields)
	sortBy!: typeof SortableFields[number];

	@IsIn(SortDirections)
	order!: typeof SortDirections[number];
}

export interface ISearchLevel {
	id: number;
	code: string;
	author: string;
	title: string;
	description: string;
	createdAt: number;
	updatedAt: number;
	downloads: number;
}

/* what is returned from the search endpoint */
export interface ISearchResponse {
	levels: ISearchLevel[];
	cursor: number;
}

/* level controller */
export default class Level {
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
			opts.cursor = req.body.cursor ?? 0;
			opts.limit = req.body.limit ?? 10;
			opts.query = req.body.query ?? '';
			opts.matchTitle = req.body.matchTitle ?? true;
			opts.matchDescription = req.body.matchDescription ?? true;
			opts.sortBy = req.body.sortBy ?? 'id';
			opts.order = req.body.order ?? 'asc';
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

		const levels = await prisma.level.findMany({
			take: opts.limit,
			...(opts.cursor && {
				skip: opts.cursor < 1 ? 0 : 1,
				cursor: {
					id: opts.cursor,
				},
			}),
			where: {
				OR: [...(titleKeywordOr ?? []), ...(descriptionKeywordOr ?? [])],
			},
			orderBy: {
				[opts.sortBy]: opts.order,
			},
			include: {
				author: true,
			},
		});

		const lastLevel = levels?.[levels.length - 1];

		const ret: ISearchResponse = {
			levels: levels.map(
				(lvl): ISearchLevel => ({
					id: lvl.id,
					code: lvl.code,
					author: lvl.author.name,
					title: lvl.title,
					description: lvl.description ?? '',
					createdAt: lvl.createdAt.getTime() / 1000,
					updatedAt: lvl.updatedAt.getTime() / 1000,
					downloads: lvl.downloads,
				})
			),
			cursor: lastLevel?.id ?? 0,
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
		const id: string | undefined = req.params.id;
		if (!id) {
			return res.status(400).send({ error: 'No id specified' });
		}
		const idNumber = parseInt(id);
		if (isNaN(idNumber)) {
			return res.status(400).send({ error: `Id "${id}" is not valid` });
		}

		const level = await prisma.level.findUnique({
			where: { id: idNumber },
			include: { author: true },
		});

		if (!level) {
			return res.status(404).send({ error: `Level id "${id}" not found` });
		}
		prisma.level
			.update({
				where: {
					id: level.id,
				},
				data: {
					downloads: {
						increment: 1,
					},
				},
			})
			.catch((err) => undefined);
		return res.status(200).send({
			level: {
				id: level.id,
				author: level.author.name,
				code: level.code,
				title: level.title,
				description: level.description ?? '',
				createdAt: level.createdAt.getTime() / 1000,
				updatedAt: level.updatedAt.getTime() / 1000,
				downloads: level.downloads + 1,
			} as ISearchLevel,
		});
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

		const user = await prisma.user.findFirst({
			where: {
				name: token.username,
			},
		});
		if (!user) {
			return res.status(500).send({
				error: 'Internal server error (NO_USER_FOUND)',
			});
		}

		const existingLevel = await prisma.level.findFirst({ where: { title } });
		if (existingLevel) {
			if (user.id !== existingLevel.authorId) {
				return res
					.status(401)
					.send({ error: 'Cannot overwrite a level you are not the author of.' });
			}
			if (!overwrite) {
				return res.status(409).send({ error: 'A level with that title already exists.' });
			}
			const updatedLevel = await prisma.level.update({
				where: { id: existingLevel.id },
				data: {
					title,
					description,
					code,
				},
				include: { author: true },
			});
			if (!updatedLevel)
				return res.status(500).send({ error: 'Internal server error (NO_OVERWRITE_LEVEL)' });
			log.info(
				`Level ${updatedLevel.title}#${updatedLevel.id} updated by user ${updatedLevel.author.name} (${updatedLevel.author.email})`
			);
			return res.status(200).send({
				level: {
					id: updatedLevel.id,
					author: updatedLevel.author.name,
					code: updatedLevel.code,
					title: updatedLevel.title,
					description: updatedLevel.description ?? '',
					createdAt: updatedLevel.createdAt.getTime() / 1000,
					updatedAt: updatedLevel.updatedAt.getTime() / 1000,
					downloads: updatedLevel.downloads,
				} as ISearchLevel,
			});
		}

		const newLevel = await prisma.level.create({
			data: {
				code,
				author: { connect: { id: user.id } },
				title,
				description,
			},
			include: { author: true },
		});

		if (!newLevel) {
			return res.status(500).send({ error: 'Internal server error (NO_POST_LEVEL)' });
		}

		log.info(
			`Level ${newLevel.title}#${newLevel.id} uploaded by user ${newLevel.author.name} (${newLevel.author.email})`
		);

		return res.status(200).send({
			level: {
				id: newLevel.id,
				author: newLevel.author.name,
				code: newLevel.code,
				title: newLevel.title,
				description: newLevel.description ?? '',
				createdAt: newLevel.createdAt.getTime() / 1000,
				updatedAt: newLevel.updatedAt.getTime() / 1000,
				downloads: newLevel.downloads,
			} as ISearchLevel,
		});
	}
}
