import { Request, Response } from 'express';
import validator from 'validator';
import dayjs from 'dayjs';

import { prisma } from '@db/index';

import { generateJwt, IAuthToken, saltAndHash, validatePassword } from '@util/tools';
import * as mail from '@util/mail';
import log from '@/log';
import { User } from '@prisma/client';

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
				code: `${Math.floor(100000 + Math.random() * 900000)}`,
				codeIssued: dayjs().toDate(),
				confirmed: false,
			},
		});

		if (!user) {
			return res.status(500).send({
				error: 'Internal server error.',
			});
		}
		// send email
		try {
			await mail.sendVerificationEmail(user.email, user.code);
			return res.status(200).send({
				jwt: generateJwt(user.name, user.tier, user.confirmed),
				confirmed: user.confirmed,
			});
		} catch (e) {
			return res.status(500).send({ error: 'Internal server error (NO_SEND_CODE)' });
		}
	}

	static async Verify(req: Request, res: Response) {
		// check and validate code
		let { code } = req.params;
		if (code == null) return res.status(400).send({ error: 'No code specified.' });
		if (!validator.isInt(code, { allow_leading_zeroes: false, min: 100000, max: 999999 }))
			return res.status(400).send({ error: 'Invalid code specified.' });

		// fetch user
		const token: IAuthToken = res.locals.token;
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
		if (user.confirmed === true) {
			return res.status(200).send({
				jwt: generateJwt(user.name, user.tier, user.confirmed),
				confirmed: true,
			});
		}
		// validate code
		if (user.code != code) return res.status(400).send({ error: 'Code does not match.' });
		// resend new code if expired
		if (dayjs().diff(user.codeIssued, 'minute') >= 10) {
			const newUser = await mail.updateCode(user.name);
			if (!newUser) {
				return res.status(500).send({
					error: 'Internal server error (NO_UPDATE_CODE)',
				});
			}
			try {
				await mail.sendVerificationEmail(newUser.email, newUser.code);
				return res.status(400).send({
					error: `Code has expired. New code has been sent to ${newUser.email}. It expires in 10 minutes.`,
				});
			} catch (e) {
				return res.status(500).send({ error: 'Internal server error (NO_RESEND_CODE)' });
			}
		}
		// set user as confirmed and return new jwt
		try {
			const confirmedUser = await prisma.user.update({
				where: {
					name: token.username,
				},
				data: {
					code: null,
					confirmed: true,
				},
			});
			log.info(`User ${confirmedUser.name} has been verified!`);
			return res.status(200).send({
				jwt: generateJwt(confirmedUser.name, confirmedUser.tier, confirmedUser.confirmed),
				confirmed: true,
			});
		} catch (e) {
			log.error(`Verification error: ${e}`);
			return res.status(500).send({
				error: 'Internal server error (NO_UPDATE_USER)',
			});
		}
	}

	static async ResendVerify(req: Request, res: Response) {
		const token: IAuthToken = res.locals.token;
		const user = await prisma.user.findFirst({
			where: {
				name: token.username,
			},
		});
		// if the user is confirmed already, ignore
		if (user.confirmed) {
			return res
				.status(400)
				.send({ error: 'This account is already verified. Try logging in again. ' });
		}
		const codeIssuedSecondsAgo = dayjs().diff(user.codeIssued, 'seconds');
		if (user.code != null && codeIssuedSecondsAgo < 30) {
			return res.status(420).send({
				error: `A code was just sent ${codeIssuedSecondsAgo} seconds ago. Please wait ${
					30 - codeIssuedSecondsAgo
				} seconds before retrying.`,
			});
		}
		// create the new code
		const newUser = await mail.updateCode(user.name);
		if (!newUser) {
			return res.status(500).send({
				error: 'Internal server error (NO_UPDATE_CODE)',
			});
		}
		// send it
		try {
			await mail.sendVerificationEmail(newUser.email, newUser.code);
			return res.status(200).send({
				message: `New code has been sent to ${newUser.email}. It expires in 10 minutes.`,
			});
		} catch (e) {
			return res.status(500).send({ error: 'Internal server error (NO_RESEND_CODE)' });
		}
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
		const codeIssuedSecondsAgo = dayjs().diff(user.codeIssued, 'seconds');
		// send a new verification email if necessary
		if (user.code == null || codeIssuedSecondsAgo >= 30) {
			// create the new code if necessary
			const newUser = await mail.updateCode(user.name);
			if (!newUser) {
				return res.status(500).send({
					error: 'Internal server error (NO_UPDATE_CODE)',
				});
			}
			// send it
			try {
				await mail.sendVerificationEmail(newUser?.email ?? user.email, newUser.code ?? user.code);
			} catch (e) {
				return res.status(500).send({ error: 'Internal server error (NO_RESEND_CODE)' });
			}
		}
		return res.status(200).send({
			jwt: generateJwt(user.name, user.tier, user.confirmed),
			confirmed: user.confirmed,
		});
	}
}
