import nodemailer from 'nodemailer';

import { prisma } from '@db/index';
import { User } from '@prisma/client';
import { Dayjs } from 'dayjs';

export const mailer = nodemailer.createTransport({
	host: process.env.SMTP_HOST,
	port: parseInt(process.env.SMTP_PORT),
	secure: false,
	auth: {
		user: process.env.SMTP_USERNAME,
		pass: process.env.SMTP_PASSWORD,
	},
});

export async function sendPasswordResetEmail(email: string, code: string, issued: Dayjs) {
	mailer.sendMail({
		from: `${process.env.SMTP_FROM_NAME} <${process.env.SMTP_FROM_EMAIL}>`,
		to: email,
		subject: 'Blockquest-Remake password reset code',
		html: `Your password reset code is: <tt>${code}</tt><br />Do not share this code with anyone.<br />This code will expire at ${issued
			.add(10, 'minutes')
			.format('MMMM D YYYY, h:mm:ss a')}`,
	});
}

export async function updateCode(name: string): Promise<User | undefined> {
	try {
		return await prisma.user.update({
			where: {
				name,
			},
			data: {
				codeIssued: new Date(),
				code: `${Math.floor(100000 + Math.random() * 900000)}`,
			},
		});
	} catch (e) {
		return undefined;
	}
}

export async function sendVerificationEmail(email: string, code: string) {
	mailer.sendMail({
		from: `${process.env.SMTP_FROM_NAME} <${process.env.SMTP_FROM_EMAIL}>`,
		to: email,
		subject: 'Verify your Blockquest-Remake email',
		text: `Your verification code is: ${code}`,
	});
}
