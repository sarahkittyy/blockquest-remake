import { Request, Response } from 'express';
import validator from 'validator';

import isValidLevel from '@util/isValidLevel';
import * as auth from '@util/passwords';

import { prisma } from '@db/index';
import log from '@/log';

/* level controller */
export default class Level {
	static async get(req: Request, res: Response) {
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

	static async upload(req: Request, res: Response) {
		const code: string | undefined = req.body.code;
		const overwrite: boolean = req.params.confirm === 'confirm';
		if (!code || !isValidLevel(code)) {
			return res.status(400).send({ error: 'Invalid level code.' });
		}

		const token: auth.IAuthToken | undefined = res.locals.token;
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
