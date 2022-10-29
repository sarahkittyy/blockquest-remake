import { Request, Response } from 'express';
import validator from 'validator';

import * as tools from '@util/tools';

import { prisma } from '@db/index';
import { Prisma } from '@prisma/client';
import log from '@/log';

import { IsBoolean, IsInt, IsNumber, IsOptional, IsString, Max, Min } from 'class-validator';

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
	matchTitle: boolean;

	@IsBoolean({ message: 'Malformed matchDescription boolean' })
	matchDescription: boolean;

	@IsOptional()
	@IsBoolean
	sortBy?: 'string
}

/* what is returned from the search endpoint */
export interface ISearchResponse {
	levels: Prisma.LevelSelect;
	cursor: number;
}

/* level controller */
export default class Level {
	/**
	 * search through all levels
	 *
	 * @static
	 * @async
	 * @param {ISearchOptions} req.body
	 */
	static async search(req: Request, res: Response) {
		let opts: ISearchOptions;
		try {
			opts = JSON.parse(req.body);
		} catch (e) {
			return res.status(400).send({ error: 'Malformed Request Body' });
		}

		if (isNaN(opts.cursor)) return res.status(400).send({ error: 'Malformed cursor' });
		opts.limit = parseInt(req.body.limit) ?? '10';
		if (isNaN(opts.limit)) return res.status(400).send({ error: 'Malformed limit' });
		opts.keywords = req.body.keywords?.split(' ');
		opts.matchTitle = req.body.matchTitle ?? true;
		opts.matchDescription = req.body.matchDescription ?? true;
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
			return res.status(400).send({ error: `Id ${id} is not valid` });
		}

		const level = await prisma.level.findUnique({
			where: { id: idNumber },
			include: { author: true },
		});

		if (!level) {
			return res.status(404).send({ error: `Level id ${id} not found` });
		}
		return res.status(200).send({
			level: {
				id: level.id,
				author: level.author.name,
				code: level.code,
				title: level.name,
				description: level.description,
				createdAt: level.createdAt.getTime() / 1000,
				updatedAt: level.updatedAt.getTime() / 1000,
			},
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

		const existingLevel = await prisma.level.findFirst({ where: { name: title } });
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
					name: title,
					description,
					code,
				},
				include: { author: true },
			});
			if (!updatedLevel)
				return res.status(500).send({ error: 'Internal server error (NO_OVERWRITE_LEVEL)' });
			log.info(
				`Level ${updatedLevel.name}#${updatedLevel.id} updated by user ${updatedLevel.author.name} (${updatedLevel.author.email})`
			);
			return res.status(200).send({
				level: {
					id: updatedLevel.id,
					author: updatedLevel.author.name,
					code: updatedLevel.code,
					title: updatedLevel.name,
					description: updatedLevel.description,
					createdAt: updatedLevel.createdAt.getTime() / 1000,
					updatedAt: updatedLevel.updatedAt.getTime() / 1000,
				},
			});
		}

		const newLevel = await prisma.level.create({
			data: {
				code,
				author: { connect: { id: user.id } },
				name: title,
				description,
			},
			include: { author: true },
		});

		if (!newLevel) {
			return res.status(500).send({ error: 'Internal server error (NO_POST_LEVEL)' });
		}

		log.info(
			`Level ${newLevel.name}#${newLevel.id} uploaded by user ${newLevel.author.name} (${newLevel.author.email})`
		);

		return res.status(200).send({
			level: {
				id: newLevel.id,
				author: newLevel.author.name,
				code: newLevel.code,
				title: newLevel.name,
				description: newLevel.description,
				createdAt: newLevel.createdAt.getTime() / 1000,
				updatedAt: newLevel.updatedAt.getTime() / 1000,
			},
		});
	}
}
