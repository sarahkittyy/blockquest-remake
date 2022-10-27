import bcrypt from 'bcrypt';
import { Request, Response, NextFunction } from 'express';
import jwt from 'jsonwebtoken';

import log from '@/log';

export async function saltAndHash(password: string): Promise<string | undefined> {
	try {
		const salt = await bcrypt.genSalt(10);
		const hash = await bcrypt.hash(password, salt);
		return hash;
	} catch (e) {
		return undefined;
	}
}

export function validatePassword(given: string, saved: string): boolean {
	return bcrypt.compareSync(given, saved);
}

export function generateJwt(username: string): string {
	const secret = process.env.SECRET ?? 'NOTSECRET';
	return jwt.sign(
		{
			username,
		},
		secret,
		{
			expiresIn: '48h',
		}
	);
}

export function getUsername(token: string | undefined): string | undefined {
	if (!token) return undefined;
	try {
		const decoded = jwt.verify(token, process.env.SECRET ?? 'NOTSECRET');
		return (decoded as any).username;
	} catch (e) {
		return undefined;
	}
}

export async function requireAuth(req: Request, res: Response, next: NextFunction) {
	const username = getUsername(req.body?.jwt);
	if (!username) {
		return res.status(401).send({ error: 'Not logged in' });
	}
	res.locals.name = username;
	next();
}
