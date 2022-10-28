require('module-alias/register');

import express, { Request, Response } from 'express';
import log from '@/log';

import Auth from '@controllers/Auth';

import { requireAuth } from '@util/passwords';

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.post('/signup', Auth.CreateAccount);
app.post('/login', Auth.Login);

app.get('/me', requireAuth(0), (req: Request, res: Response) => {
	return res.send(res.locals.token);
});

const port = process.env.PORT ?? '3000';
app.listen(parseInt(port), () => {
	log.info(`Listening on port ${port}`);
});
