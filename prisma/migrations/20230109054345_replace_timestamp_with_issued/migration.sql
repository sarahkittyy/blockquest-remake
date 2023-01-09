/*
  Warnings:

  - You are about to drop the column `createdAt` on the `UserPasswordReset` table. All the data in the column will be lost.
  - You are about to drop the column `updatedAt` on the `UserPasswordReset` table. All the data in the column will be lost.

*/
-- AlterTable
ALTER TABLE "UserPasswordReset" DROP COLUMN "createdAt",
DROP COLUMN "updatedAt",
ADD COLUMN     "issued" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP;
