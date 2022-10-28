/*
  Warnings:

  - Added the required column `description` to the `Level` table without a default value. This is not possible if the table is not empty.
  - Added the required column `name` to the `Level` table without a default value. This is not possible if the table is not empty.

*/
-- AlterTable
ALTER TABLE "Level" ADD COLUMN     "description" TEXT NOT NULL,
ADD COLUMN     "name" VARCHAR(50) NOT NULL;
