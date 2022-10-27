import { Request, Response } from 'express';
import validator from 'validator';

import { prisma } from '@db/index';

import { generateJwt, saltAndHash, validatePassword } from '@util/passwords';

export default class Auth {
	static async CreateAccount(req: Request, res: Response) {
		const { email, name, password } = req.body;

		// check for existing user
		const existingUser = await prisma.user.findFirst({
			where: {
				OR: [
					{
						email,
					},
					{
						name,
					},
				],
			},
		});
		if (existingUser != null) {
			const sameEmail = existingUser.email === email;
			const sameName = existingUser.name === name;
			return res.status(403).send({
				error: sameEmail
					? 'Account with that email already exists'
					: 'Account with that username already exists',
				field: sameEmail ? 'email' : 'name',
			});
		}

		// hash password
		const hashedPass = await saltAndHash(password);
		if (!hashedPass) {
			return res.status(500).send({
				error: 'Internal server error.',
			});
		}

		// create model
		const user = await prisma.user.create({
			data: {
				email,
				name,
				password: hashedPass,
			},
		});

		if (!user) {
			return res.status(500).send({
				error: 'Internal server error.',
			});
		}
		return res.status(200).send({
			jwt: generateJwt(user.name),
		});
	}

	static async Login(req: Request, res: Response) {
		let email: string | undefined = undefined;
		let name: string | undefined = undefined;
		if (validator.isEmail(req.body.username)) {
			email = req.body.username;
		} else {
			name = req.body.username;
		}
		const password: string = req.body.password;

		const user = await prisma.user.findFirst({
			where: {
				...(!!email && { email }),
				...(!!name && { name }),
			},
		});
		if (!user) {
			return res.status(401).send({ error: 'User / Email does not exist' });
		}
		if (!password || !user.password || !validatePassword(password, user.password)) {
			return res.status(401).send({ error: 'Invalid password' });
		}
		return res.status(200).send({
			jwt: generateJwt(user.name),
		});
	}
}
