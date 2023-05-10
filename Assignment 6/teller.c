#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"
#include "bank.h"
#include "branch.h"
/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  sem_wait(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
  sem_wait(&(account->lockStatus));
  Account_Adjust(bank, account, amount, 1);
  sem_post(&(account->lockStatus));
  sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    //sem_post(&(account->lockStatus));
    //sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  sem_wait(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
  sem_wait(&(account->lockStatus));
  
  

  if (amount > Account_Balance(account)) {
    sem_post(&(account->lockStatus));
    sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank,account, -amount, 1);
  sem_post(&(account->lockStatus));
  sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  if (srcAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if (dstAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  
  sem_wait(&(bank->branchFix));
  if (amount > Account_Balance(srcAccount)) {
    return ERROR_INSUFFICIENT_FUNDS;
  }
  sem_post(&(bank->branchFix));
  if (dstAccountNum == srcAccountNum) return ERROR_SUCCESS;

  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);

  if(updateBranch == 0){
    if(srcAccount->accountNumber < dstAccount->accountNumber){
      sem_wait(&(srcAccount->lockStatus));
      sem_wait(&(dstAccount->lockStatus));
    } else {
      sem_wait(&(dstAccount->lockStatus));
      sem_wait(&(srcAccount->lockStatus));
    }
  
    Account_Adjust(bank, srcAccount, -amount, updateBranch);
    Account_Adjust(bank, dstAccount, amount, updateBranch);

    sem_post(&(srcAccount->lockStatus));
    sem_post(&(dstAccount->lockStatus));

    return ERROR_SUCCESS;
  }else{
    int srcID = AccountNum_GetBranchID(srcAccountNum);
    int dstID = AccountNum_GetBranchID(dstAccountNum);

    if (dstID > srcID) {           
      sem_wait(&(bank->branches[srcID].branchLock));   
      sem_wait(&(bank->branches[dstID].branchLock));
      sem_wait(&(srcAccount->lockStatus));            
      sem_wait(&(dstAccount->lockStatus));     
    } else {
      sem_wait(&(bank->branches[dstID].branchLock));
      sem_wait(&(bank->branches[srcID].branchLock));
      sem_wait(&(dstAccount->lockStatus));
      sem_wait(&(srcAccount->lockStatus));
    }
    
    sem_wait(&(bank->bankLock));
    Account_Adjust(bank, srcAccount, -amount, updateBranch);
    Account_Adjust(bank, dstAccount, amount, updateBranch);
    sem_post(&(bank->bankLock));
    sem_post(&(srcAccount->lockStatus));                        
    sem_post(&(dstAccount->lockStatus));
    sem_post(&(bank->branches[srcID].branchLock));   
    sem_post(&(bank->branches[dstID].branchLock));
    return ERROR_SUCCESS;
  }
}
