require('module-alias/register');

import express, { Request, Response } from 'express';
import log from '@/log';

import Auth from '@controllers/Auth';

import validate from '@util/validate';
import { body, oneOf } from 'express-validator';
import { requireAuth } from '@util/passwords';

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.post(
	'/signup',
	[
		body('email').isEmail().normalizeEmail(),
		body('name').isLength({ min: 2, max: 30 }),
		body('password').isLength({ min: 8 }),
		validate,
	],
	Auth.CreateAccount
);

app.post(
	'/login',
	[
		oneOf([
			body('username').isEmail().normalizeEmail(),
			body('username').isLength({ min: 2, max: 30 }),
		]),
		body('password').isLength({ min: 8 }),
		validate,
	],
	Auth.Login
);

app.get('/me', [requireAuth], (req: Request, res: Response) => {
	return res.send(res.locals.name);
});

const port = process.env.PORT ?? '3000';
app.listen(parseInt(port), () => {
	log.info(`Listening on port ${port}`);
});
