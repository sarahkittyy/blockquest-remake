/*
  Warnings:

  - You are about to drop the column `emailCode` on the `User` table. All the data in the column will be lost.

*/
-- AlterTable
ALTER TABLE "User" ADD COLUMN     "code" TEXT;
