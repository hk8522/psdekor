# psdekor
Electronic door written in atmel studio


Project details: We have an existing code for an existing electronic lock. Now, a customer needs to have some changes and therefore, we need to adapt the code.

The lock is installed in a fitness centre. Normally, a user occupies a locker with an RFID card and only the user or the administrator (¡°programming card¡±) can open this locker. If the administrator opens the locker, the original used card is deleted.

New:
1. the administrator card should not delete the user card anymore.
2. If the administrator card gets placed on the lock for 10 seconds, the user card gets deleted


========
We have different functions. The changes need to be done in the GYM function ...

#define SW_FUNCTION_GYM

I assume the changes have to be bade in application.c

we have also a gym 6h, 12h and 24h > ignore them > we need it in the standard "GYM" without any hours
