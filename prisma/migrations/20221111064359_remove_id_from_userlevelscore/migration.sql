/*
  Warnings:

  - The primary key for the `UserLevelScore` table will be changed. If it partially fails, the table could be left without primary key constraint.
  - You are about to drop the column `id` on the `UserLevelScore` table. All the data in the column will be lost.

*/
-- AlterTable
ALTER TABLE "UserLevelScore" DROP CONSTRAINT "UserLevelScore_pkey",
DROP COLUMN "id",
ADD CONSTRAINT "UserLevelScore_pkey" PRIMARY KEY ("userId", "levelId");
