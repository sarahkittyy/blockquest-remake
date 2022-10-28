require('module-alias/register');

import express, { NextFunction, Request, Response } from 'express';
import log from '@/log';

import Auth from '@controllers/Auth';
import Level from '@controllers/Level';

import { requireAuth } from '@util/passwords';

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.use((req: Request, res: Response, next: NextFunction) => {
	log.info(`${req.method} ${req.originalUrl}`);
	next();
});

app.post('/signup', Auth.CreateAccount);
app.post('/login', Auth.Login);

app.post('/level/upload/:confirm?', requireAuth(0), Level.upload);
app.get('/level/:id', Level.get);

app.get('/me', requireAuth(0), (req: Request, res: Response) => {
	return res.send(res.locals.token);
});

app.get('/download', (req: Request, res: Response) => {
	return res.redirect('https://github.com/sarahkittyy/blockquest-remake/releases/latest');
});

app.use('*', (req: Request, res: Response) => {
	return res
		.status(404)
		.send(`blockquest-remake api endpoint ${req.method} ${req.originalUrl} not found`);
});

const port = process.env.PORT ?? '3000';
app.listen(parseInt(port), () => {
	log.info(`Listening on port ${port}`);
});
