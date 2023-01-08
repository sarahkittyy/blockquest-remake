/*
  Warnings:

  - A unique constraint covering the columns `[verificationId]` on the table `Level` will be added. If there are existing duplicate values, this will fail.

*/
-- AlterTable
ALTER TABLE "Level" ADD COLUMN     "verificationId" INTEGER;

-- CreateIndex
CREATE UNIQUE INDEX "Level_verificationId_key" ON "Level"("verificationId");

-- AddForeignKey
ALTER TABLE "Level" ADD CONSTRAINT "Level_verificationId_fkey" FOREIGN KEY ("verificationId") REFERENCES "UserLevelScore"("id") ON DELETE SET NULL ON UPDATE CASCADE;
