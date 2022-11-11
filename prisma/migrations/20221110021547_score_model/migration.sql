-- CreateTable
CREATE TABLE "UserLevelScore" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "userId" INTEGER NOT NULL,
    "levelId" INTEGER NOT NULL,
    "time" DOUBLE PRECISION NOT NULL,
    "version" VARCHAR(10) NOT NULL,
    "replay" BYTEA NOT NULL,

    CONSTRAINT "UserLevelScore_pkey" PRIMARY KEY ("id")
);

-- AddForeignKey
ALTER TABLE "UserLevelScore" ADD CONSTRAINT "UserLevelScore_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "UserLevelScore" ADD CONSTRAINT "UserLevelScore_levelId_fkey" FOREIGN KEY ("levelId") REFERENCES "Level"("id") ON DELETE RESTRICT ON UPDATE CASCADE;
