/*
  Warnings:

  - You are about to drop the column `name` on the `Level` table. All the data in the column will be lost.
  - A unique constraint covering the columns `[title]` on the table `Level` will be added. If there are existing duplicate values, this will fail.
  - Added the required column `title` to the `Level` table without a default value. This is not possible if the table is not empty.

*/
-- AlterTable
ALTER TABLE "Level" RENAME COLUMN "name" TO "title"
