/*
  Warnings:

  - A unique constraint covering the columns `[id]` on the table `UserLevelScore` will be added. If there are existing duplicate values, this will fail.

*/
-- AlterTable
ALTER TABLE "UserLevelScore" ADD COLUMN     "id" SERIAL NOT NULL;

-- CreateIndex
CREATE UNIQUE INDEX "UserLevelScore_id_key" ON "UserLevelScore"("id");
