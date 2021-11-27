# Classic Parental Control (for Windows)

The project provides uitilities which allow you to establish/maintain Parental Control for Local Accounts in Windows 10.
- `LogonHoursService` monitors allowed logon hours (which you typically can set with a command like `net user USERNAME /time:M-F,10-18`) and locks the session once the time is over

## Compilation
You need VS2019 with vc142 toolkit to build the solution.
