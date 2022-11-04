-- CreateTable
CREATE TABLE "UserLevelVote" (
    "userId" INTEGER NOT NULL,
    "levelId" INTEGER NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "vote" INTEGER NOT NULL,

    CONSTRAINT "UserLevelVote_pkey" PRIMARY KEY ("userId","levelId")
);

-- AddForeignKey
ALTER TABLE "UserLevelVote" ADD CONSTRAINT "UserLevelVote_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "UserLevelVote" ADD CONSTRAINT "UserLevelVote_levelId_fkey" FOREIGN KEY ("levelId") REFERENCES "Level"("id") ON DELETE RESTRICT ON UPDATE CASCADE;
