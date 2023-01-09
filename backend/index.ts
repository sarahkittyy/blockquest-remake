require('module-alias/register');

import express, { NextFunction, Request, Response } from 'express';
import log from '@/log';

import Auth from '@controllers/Auth';
import Level from '@controllers/Level';
import Replay from '@controllers/Replay';
import Comment from '@controllers/Comment';
import User from '@controllers/User';

import { checkAuth, requireAuth } from '@util/tools';

import { mailer } from '@util/mail';

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.use((req: Request, res: Response, next: NextFunction) => {
	log.info(`${req.method} ${req.originalUrl}`);
	next();
});

app.post('/signup', Auth.CreateAccount);
app.post('/login', Auth.Login);
app.post('/resend-verify', requireAuth(0, false), Auth.ResendVerify);
app.post('/verify/:code', requireAuth(0, false), Auth.Verify);
app.post('/forgot-password', Auth.ForgotPassword);
app.post('/reset-password', Auth.ResetPassword);

app.post('/level/upload/:confirm?', requireAuth(0), Level.upload);
app.post('/level/search', checkAuth(), Level.search);
app.get('/level/quickplay', checkAuth(), Level.getQuickplay);
app.get('/level/:id/ping-download', Level.downloadPing);
app.post('/level/:id(\\d+)/:vote(like|dislike)', requireAuth(0), Level.vote);
app.post('/level/:id', checkAuth(), Level.getById);

app.post('/replay/upload', requireAuth(0), Replay.upload);
app.post('/replay/search/:levelId(\\d+)', checkAuth(), Replay.search);
app.post('/replay/:id(\\d+)', checkAuth(), Replay.get);
app.post('/replay/:id(\\d+)/:hide(hide|unhide)', requireAuth(0), Replay.hide);

app.post('/comments/level/:levelId(\\d+)', Comment.get);
app.post('/comments/new/:levelId(\\d+)', requireAuth(0), Comment.post);

app.post('/users/:id(\\d+)', checkAuth(), User.get);
app.post('/users/name/:name', checkAuth(), User.getByName);

app.get('/token', requireAuth(0), (req: Request, res: Response) => {
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
