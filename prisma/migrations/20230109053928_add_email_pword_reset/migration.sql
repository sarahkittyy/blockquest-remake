/*
  Warnings:

  - A unique constraint covering the columns `[email]` on the table `UserPasswordReset` will be added. If there are existing duplicate values, this will fail.
  - Added the required column `email` to the `UserPasswordReset` table without a default value. This is not possible if the table is not empty.

*/
-- AlterTable
ALTER TABLE "UserPasswordReset" ADD COLUMN     "email" TEXT NOT NULL;

-- CreateIndex
CREATE UNIQUE INDEX "UserPasswordReset_email_key" ON "UserPasswordReset"("email");
