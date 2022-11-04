import { PrismaClient, User, Level, UserLevelVote } from '@prisma/client';
import {
	randUserName,
	randPassword,
	randEmail,
	randAlphaNumeric,
	randTextRange,
	randText,
	randPhrase,
	randNumber,
} from '@ngneat/falso';

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
			code: randTextRange({ min: 1024, max: 4000 }),
			title: randText({ charCount: 15 }),
			description: randPhrase(),
			author: { connect: { id: author.id } },
			downloads: randNumber({ min: 0, max: 8900 }),
		},
	});
	return level;
}

async function main() {
	const userCount = Math.floor(Math.random() * 10) + 5;
	for (let i = 0; i < userCount; ++i) {
		const levelCount = Math.floor(Math.random() * 4);
		const user = await fakeUser();
		for (let j = 0; j < levelCount; ++j) {
			const level = await fakeLevel(user);
			await fakeVote(user, level);
		}
	}
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
