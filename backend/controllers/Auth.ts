import { Request, Response } from 'express';
import validator from 'validator';

import { prisma } from '@db/index';

import { generateJwt, saltAndHash, validatePassword } from '@util/tools';

export default class Auth {
	static async CreateAccount(req: Request, res: Response) {
		let { email, name, password } = req.body;

		if (email == null) return res.status(400).send({ error: 'No email provided' });
		if (name == null) return res.status(400).send({ error: 'No username provided' });
		if (password == null) return res.status(400).send({ error: 'No password provided' });

		if (!validator.isEmail(email)) return res.status(400).send({ error: 'Invalid email address' });
		email = validator.normalizeEmail(email);
		if (!validator.isLength(name, { min: 2, max: 30 }))
			return res.status(400).send({ error: 'Username too short / too long.' });
		if (!validator.isLength(password, { min: 8 }))
			return res.status(400).send({ error: 'Password must be at least 8 characters long' });

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
			jwt: generateJwt(user.name, user.tier),
		});
	}

	static async Login(req: Request, res: Response) {
		let email: string | undefined = undefined;
		let name: string | undefined = undefined;
		if (validator.isEmail(req.body.username)) {
			email = validator.normalizeEmail(req.body.username) as string;
		} else {
			name = req.body.username;
		}
		if (!name && !email) return res.status(400).send({ error: 'No username / email specified' });
		const password: string | undefined = req.body.password;
		if (!password) return res.status(400).send({ error: 'No password specified' });

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
			jwt: generateJwt(user.name, user.tier),
		});
	}
}
