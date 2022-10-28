/*
  Warnings:

  - You are about to drop the column `perms` on the `User` table. All the data in the column will be lost.

*/
-- AlterTable
ALTER TABLE "User" DROP COLUMN "perms",
ADD COLUMN     "tier" INTEGER NOT NULL DEFAULT 0;
