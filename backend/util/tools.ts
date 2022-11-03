import bcrypt from 'bcrypt';
import { Request, Response, NextFunction } from 'express';
import jwt from 'jsonwebtoken';

import log from '@/log';

export function isValidLevel(code: string): boolean {
	const levelRegex = /^(?:[0-9]{3}|\/){1024}/g;
	if (code.match(/^[/0-9]{1023,3073}$/) == null) return false;
	if (code.match(levelRegex) == null) return false;
	return true;
}

export interface IAuthToken {
	username: string;
	tier: number;
	confirmed: boolean;
}

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

export function generateJwt(username: string, tier: number, confirmed: boolean): string {
	const secret = process.env.SECRET ?? 'NOTSECRET';
	return jwt.sign(
		{
			username,
			tier,
			confirmed,
		},
		secret,
		{
			expiresIn: '48h',
		}
	);
}

export function decodeToken(token: string): IAuthToken | undefined {
	if (!token) return undefined;
	try {
		const decoded = jwt.verify(token, process.env.SECRET ?? 'NOTSECRET');
		return decoded as IAuthToken;
	} catch (e) {
		return undefined;
	}
}

export function requireAuth(tier = 0, confirmed = true) {
	return async (req: Request, res: Response, next: NextFunction) => {
		const token: IAuthToken | undefined = decodeToken(req.body?.jwt);
		if (!token) {
			return res.status(401).send({ error: 'Not logged in' });
		}
		if (!token.confirmed && confirmed) {
			return res.status(409).send({ error: 'Email not verified ' });
		}
		if (token.tier < tier) {
			return res.status(401).send({ error: 'Insufficient permissions' });
		}
		res.locals.token = token;
		next();
	};
}
