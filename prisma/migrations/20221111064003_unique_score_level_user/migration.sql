/*
  Warnings:

  - The primary key for the `UserLevelScore` table will be changed. If it partially fails, the table could be left without primary key constraint.

*/
-- AlterTable
ALTER TABLE "UserLevelScore" DROP CONSTRAINT "UserLevelScore_pkey",
ADD CONSTRAINT "UserLevelScore_pkey" PRIMARY KEY ("id", "userId", "levelId");
