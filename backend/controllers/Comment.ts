import { UserLevelComment } from '@prisma/client';
import { IsIn, IsInt, IsNumber, IsOptional, Max, Min, validate } from 'class-validator';
import { Request, Response } from 'express';
import { prisma } from '@db/index';

import * as tools from '@util/tools';
import dayjs from 'dayjs';

export interface ICommentResponse {
	user: tools.UserStub;
	id: number;
	levelId: number;
	text: string;
	createdAt: number;
	updatedAt: number;
}

const SortableFields = ['createdAt'] as const;
const SortDirections = ['asc', 'desc'] as const;

export class ISearchOptions {
	@IsOptional()
	@IsNumber(undefined, { message: 'Malformed cursor' })
	cursor?: number; // current comment index to search past

	@IsInt({ message: 'Malformed limit' })
	@Min(1)
	@Max(20)
	limit!: number; // how many entries to fetch

	@IsIn(SortableFields)
	sortBy!: typeof SortableFields[number];

	@IsIn(SortDirections)
	order!: typeof SortDirections[number];
}
export interface ISearchResponse {
	comments: ICommentResponse[];
	cursor: number;
}

export default class Comment {
	/**
	 * search through all comments given a level
	 *
	 * @static
	 * @async
	 * @param {ISearchOptions} req.body
	 * @param {levelId} req.params
	 */
	static async get(req: Request, res: Response) {
		let opts: ISearchOptions = new ISearchOptions();
		try {
			opts.cursor = req.body.cursor ?? -1;
			opts.limit = req.body.limit ?? 10;
			opts.sortBy = req.body.sortBy ?? 'createdAt';
			opts.order = req.body.order ?? 'desc';
			const errors = await validate(opts);
			if (errors?.length > 0) {
				return res.status(400).send({
					error: Object.entries(errors[0].constraints ?? [[0, 'Malformed Request Body']])[0][1],
				});
			}
		} catch (e) {
			return res.status(400).send({ error: 'Malformed Request Body' });
		}
		const levelId = parseInt(req.params.levelId ?? 'nan');
		if (isNaN(levelId)) {
			return res.status(400).send({ error: 'Invalid levelId parameter.' });
		}
		const comments = await prisma.userLevelComment.findMany({
			take: opts.limit,
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
		if (comments.length === 0) {
			return res.status(200).send({
				comments: [],
				cursor: -1,
			});
		}
		const lastComment = comments[comments.length - 1];
		const ret: ISearchResponse = {
			comments: comments.map((comment) => tools.toCommentResponse(comment)),
			cursor: lastComment?.id && comments.length >= opts.limit ? lastComment.id : -1,
		};
		return res.status(200).send(ret);
	}

	static async post(req: Request, res: Response) {
		const levelId = parseInt(req.params.levelId ?? 'nan');
		if (isNaN(levelId)) {
			return res.status(400).send({ error: 'Invalid levelId parameter.' });
		}
		const token: tools.IAuthToken = res.locals.token;
		let comment: string = req.body.comment?.trim();
		if (!comment || comment.length === 0)
			return res.status(400).send({ error: 'No comment provided.' });
		if (comment.length > 250) return res.status(400).send({ error: 'Comment too long.' });
		// check last comment posted by this user
		const lastComment: UserLevelComment | undefined = await prisma.userLevelComment.findFirst({
			where: {
				userId: token.id,
				levelId,
			},
			orderBy: {
				createdAt: 'desc',
			},
		});
		if (lastComment) {
			const secondsSinceLastCommentPosted = dayjs().diff(lastComment.createdAt, 'seconds');
			if (secondsSinceLastCommentPosted < 30) {
				return res.status(429).send({
					error: `Too fast! Wait ${
						30 - secondsSinceLastCommentPosted
					} more seconds before posting again.`,
				});
			}
		}

		try {
			const newComment = await prisma.userLevelComment.create({
				data: {
					comment,
					level: {
						connect: {
							id: levelId,
						},
					},
					user: {
						connect: {
							id: token.id,
						},
					},
				},
				include: {
					user: true,
				},
			});
			return res.status(200).send({
				comment: tools.toCommentResponse(newComment),
			});
		} catch (e) {
			return res.status(400).send({
				error: 'Internal server error (NO_POST_COMMENT)',
			});
		}
	}
}
