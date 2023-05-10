#ifndef _BANK_H
#define _BANK_H

#include <semaphore.h>

typedef struct Bank {
  unsigned int numberBranches;
  unsigned int numberWorkersHasToFinish;
  struct       Branch  *branches;
  struct       Report  *report;
  sem_t checker;
  sem_t startNextDay;
  sem_t reportLock; 
  sem_t quickLock;
  sem_t bankLock;
  sem_t branchFix;
} Bank;

#include "account.h"

int Bank_Balance(Bank *bank, AccountAmount *balance);

Bank *Bank_Init(int numBranches, int numAccounts, AccountAmount initAmount,
                AccountAmount reportingAmount,
                int numWorkers);

int Bank_Validate(Bank *bank);
int Bank_Compare(Bank *bank1, Bank *bank2);



#endif /* _BANK_H */
