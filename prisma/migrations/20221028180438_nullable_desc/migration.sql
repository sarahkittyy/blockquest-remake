/*
  Warnings:

  - A unique constraint covering the columns `[name]` on the table `Level` will be added. If there are existing duplicate values, this will fail.

*/
-- AlterTable
ALTER TABLE "Level" ALTER COLUMN "description" DROP NOT NULL;

-- CreateIndex
CREATE UNIQUE INDEX "Level_name_key" ON "Level"("name");
