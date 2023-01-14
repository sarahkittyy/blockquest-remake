-- AlterTable
ALTER TABLE "User" ADD COLUMN     "pinnedId" INTEGER;

-- AddForeignKey
ALTER TABLE "User" ADD CONSTRAINT "User_pinnedId_fkey" FOREIGN KEY ("pinnedId") REFERENCES "Level"("id") ON DELETE SET NULL ON UPDATE CASCADE;
