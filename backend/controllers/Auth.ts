import { Request, Response } from 'express';
import validator from 'validator';
import dayjs from 'dayjs';

import { prisma } from '@db/index';

import { generateJwt, IAuthToken, saltAndHash, validatePassword } from '@util/tools';
import * as mail from '@util/mail';
import * as tools from '@util/tools';
import * as constants from '@util/constants';
import log from '@/log';
import { stringify } from 'flatted';

export default class Auth {
	static async ForgotPassword(req: Request, res: Response) {
		let { email } = req.body;
		if (email == null) return res.status(400).send({ error: 'No email provided.' });
		if (!validator.isEmail(email)) return res.status(400).send({ error: 'Invalid email address' });
		email = validator.normalizeEmail(email);

		try {
			// try to find an existing one
			const existingReset = await prisma.userPasswordReset.findUnique({
				where: {
					email,
				},
			});
			if (
				existingReset != null &&
				dayjs().diff(existingReset.issued, 'minutes') < constants.PASSWORD_RESET_EXP_MINUTES
			) {
				return res.status(200).send({
					exists: true,
					exp: dayjs(existingReset.issued)
						.add(constants.PASSWORD_RESET_EXP_MINUTES, 'minutes')
						.unix(),
				});
			}
			// upsert the password reset model
			const passwordReset = await prisma.userPasswordReset.upsert({
				where: {
					email,
				},
				update: {
					issued: new Date(),
					code: tools.randomString(64),
				},
				create: {
					user: {
						connect: {
							email,
						},
					},
					email,
					code: tools.randomString(64),
					issued: new Date(),
				},
			});
			if (passwordReset == null) {
				return res.status(400).send({ error: 'Email not found.' });
			}

			// send the reset email
			mail.sendPasswordResetEmail(email, passwordReset.code, dayjs(passwordReset.issued));

			return res.status(200).send({
				exists: false,
				exp: dayjs(passwordReset.issued)
					.add(constants.PASSWORD_RESET_EXP_MINUTES, 'minutes')
					.unix(),
			});
		} catch (e) {
			return res.status(400).send({ error: 'Email not found.' });
		}
	}

	static async ResetPassword(req: Request, res: Response) {
		let { email, code, password } = req.body;
		if (email == null) return res.status(400).send({ error: 'No email provided' });
		if (code == null) return res.status(400).send({ error: 'No code provided' });
		if (password == null) return res.status(400).send({ error: 'No new password provided' });

		if (!validator.isEmail(email)) return res.status(400).send({ error: 'Invalid email address' });
		email = validator.normalizeEmail(email);
		if (!validator.isLength(password, { min: 8 }))
			return res.status(400).send({ error: 'Password must be at least 8 characters long' });
		if (`${code}`.length != 64) return res.status(400).send({ error: 'Invalid code' });

		// fetch the reset model
		const reset = await prisma.userPasswordReset.findUnique({
			where: {
				email,
			},
		});
		if (reset == null)
			return res.status(400).send({ error: 'Email address not found / has no pending reset.' });
		// send a new code if the current one is expired
		if (dayjs().diff(reset.issued, 'minutes') > constants.PASSWORD_RESET_EXP_MINUTES) {
			const newReset = await prisma.userPasswordReset.update({
				where: {
					email,
				},
				data: {
					issued: new Date(),
					code: tools.randomString(64),
				},
			});
			mail.sendPasswordResetEmail(email, newReset.code, dayjs(newReset.issued));
			return res
				.status(400)
				.send({ error: 'Code expired. A new code has been sent to your inbox.' });
		}
		if (reset.code !== code) return res.status(400).send({ error: 'Incorrect code.' });

		// update the password
		const newPasswordHashed = await tools.saltAndHash(password);
		if (newPasswordHashed == null)
			return res.status(500).send({ error: 'Internal server error (NO_HASH)' });
		try {
			const newUser = await prisma.user.update({
				where: {
					id: reset.userId,
				},
				data: {
					password: newPasswordHashed,
				},
			});

			if (newUser == null) throw 'Internal server error (NO_NEW_USER)';

			await prisma.userPasswordReset.delete({
				where: {
					userId: newUser.id,
				},
			});

			return res.status(200).send();
		} catch (e) {
			return res.status(500).send({ error: stringify(e) });
		}
	}

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
				jwt: generateJwt(user),
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
				jwt: generateJwt(user),
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
				jwt: generateJwt(confirmedUser),
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
		if (!user.confirmed && (user.code == null || codeIssuedSecondsAgo >= 30)) {
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
			jwt: generateJwt(user),
			confirmed: user.confirmed,
		});
	}
}
