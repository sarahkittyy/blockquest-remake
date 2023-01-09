import { PrismaClient, User, Level, UserLevelVote, UserLevelComment } from '@prisma/client';
import {
	randUserName,
	randPassword,
	randEmail,
	randText,
	randPhrase,
	randNumber,
	randCatchPhrase,
	randRecentDate,
} from '@ngneat/falso';
import bcrypt from 'bcrypt';

const sampleLevel =
	'////////////////////////////////////////////////////////////////020/062062//020/////020///064064///020////////////////////////////////////////////////////////////////////////////////////////////////010//////////////////////020020020020020020020020020020//////////////////////020////////////////////////////030//020/////080/////////////080/000/////////020/////063/////////////063/020020020020020020////020/////063/////////////063//////020////020/////063/////////////063//////020////020/////063/////////////063//////020050050050050020/////063/////////////063/////////////////063/////////////063/////////////////063/////////////063080080080080080080080080080080080080080080080080080063/////////////063063063063063063063063063063063063063063063063063063063////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////';
const sampleScore = {
	replay:
		'djEuNS4xAAAAAAAARgAAAG53s2NzYXJhaGtpdHR5AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB7FO4/hmEYhiEIgiAIgiAIgiAIgiAIgiAIgiAIgiAIgiAIgmEQRVEURVEURVEUhmEYhmEIgiAIgiAIgiAIgmEYhmEYhmEYhmEYhmEYhmEYhmEYhmEYhmEYhiEIgiAAAAAAAAAAAAAAAAAAAAAIgiAIgiAIgiAIgiAIgiAIgiAIgiAIgiAIgiAIgiAIgiAIggAA',
	time: 1.860000014305115,
};

export async function saltAndHash(password: string): Promise<string | undefined> {
	try {
		const salt = await bcrypt.genSalt(10);
		const hash = await bcrypt.hash(password, salt);
		return hash;
	} catch (e) {
		return undefined;
	}
}

const prisma = new PrismaClient();

async function fakeUser(): Promise<User> {
	const user = await prisma.user.create({
		data: {
			name: randUserName(),
			password: randPassword({ size: 16 }),
			email: randEmail(),
			confirmed: false,
			tier: 0,
		},
	});
	return user;
}

async function fakeComment(user: User, level: Level): Promise<UserLevelComment> {
	return await prisma.userLevelComment.create({
		data: {
			comment: randCatchPhrase(),
			levelId: level.id,
			userId: user.id,
			createdAt: randRecentDate(),
		},
	});
}

async function fakeVote(user: User, level: Level): Promise<UserLevelVote> {
	return await prisma.userLevelVote.create({
		data: {
			vote: Math.random() > 0.5 ? 1 : -1,
			level: { connect: { id: level.id } },
			user: { connect: { id: user.id } },
		},
	});
}

async function fakeLevel(author: User) {
	const level = await prisma.level.create({
		data: {
			code: sampleLevel,
			title: randText({ charCount: 15 }),
			description: randPhrase(),
			author: { connect: { id: author.id } },
			downloads: randNumber({ min: 0, max: 8900 }),
		},
	});
	return level;
}

async function fakeScore(user: User, level: Level) {
	const score = prisma.userLevelScore.create({
		data: {
			user: { connect: { id: user.id } },
			level: { connect: { id: level.id } },
			time: sampleScore.time,
			replay: Buffer.from(sampleScore.replay, 'base64'),
			version: 'vSeeded',
			hidden: Math.random() > 0.7,
		},
	});
	return score;
}

async function main() {
	const userCount = Math.floor(Math.random() * 8) + 5;
	let levels: Level[] = [];
	let users: User[] = [];
	for (let i = 0; i < userCount; ++i) {
		const levelCount = Math.floor(Math.random() * 8) + 3;
		const user = await fakeUser();
		users.push(user);
		for (let j = 0; j < levelCount; ++j) {
			const level = await fakeLevel(user);
			levels.push(level);
		}
		levels.forEach(async (lvl) => {
			await fakeScore(user, lvl);
			await fakeVote(user, lvl);
		});
	}
	users.forEach(async (user) => {
		levels.forEach(async (level) => {
			const ct = Math.floor(Math.random() * 3);
			for (let i = 0; i < ct; ++i) {
				await fakeComment(user, level);
			}
		});
	});

	// test account
	await prisma.user.create({
		data: {
			name: 'dev',
			password: (await saltAndHash('dev')) ?? 'UNDEFINED',
			email: process.env.DEV_EMAIL ?? 'dev@dev.dev',
			confirmed: true,
			tier: 0,
		},
	});
}

main()
	.then(async () => {
		await prisma.$disconnect();
	})
	.catch(async (e) => {
		console.error(e);
		await prisma.$disconnect();
		process.exit(1);
	});
