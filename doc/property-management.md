# Property managment

Gnucash may help to ease the managment of your property. If you use it
for your own house or appartment, it already offers all tasks assigned
to the bookeeping part.  To handle multiple appartments or rentals in
a community property you have to get hold of some other tasks.

## Background

In Germany, a co-owners' association (WEG) is free to decide to
self-manage their property in an owner administration. Tasks of
[property][property_mgmt] management can then be taken over by an
owner of the WEG appointed for this purpose.

The rules may be adopted to the needs in other countries as well.

[property_mgmt]: https://www.hausverwaltung-ratgeber.de/hausverwaltung.html).

## Implementation

Following the management of customers and suppliers a new entity `Co-Owner`
has been introduced. It offers to

* Create
* Edit
* List
* Search

the new entity type.

Similar to customers, is enables the handling `Co-Owner` assigned

* Invoices
* Orders
* Payments
* Settlements

Within a Co-Owner object, you can define attributes that vary per
housing unit. They are used while managing property managment tasks.
For example, the following object identifiers must be assigned to
create correct settlements:

* porperty share
* apartment unit
* distribution key

With regard to accounting tasks a suitable account structure is
required. As usual you will manage the necessary bookings adressing
the given accounts.

The legal regulations that exist in Germany stipulate that individual
settlements must be created annually for the Co-Ownersi. In particular
this settlements must show and proportionately allocate

* apportionable amounts
* non-apportionable amounts
* incomes
* individual costs

as well as a report of the

* property maintenance reserve

## Distribution lists

Distribution lists allow the definition of parameters that are
required for the calculation of settlement values for the respective
co-ownership share.

The type of distribution list groups relevant attributes.

### Shares

The distribution and calculation is performed using the numerical
values of the ownership shares in relation to the sum of all shares.

* LabelSettlement: The identifier shown in a settlement.
* SharesTotal: Sum of all shares (default: 1000 shares).
* Owner: List of assigned owner entities that will be respected in settlements.
  * OwnerId: Name/Id.
  * Owner-Share: Share value

### Percentage shares

The distribution and calculation is made using the percentage values
of the ownership shares in relation to the sum of the percentage share
values.

* LabelSettlement: The identifier shown in a settlement.
* PercentageTotal: The percentage value representing all shares (default: 100%).
* Owner: List of assigned owner entities that will be respected in settlements.
* Eigentümer: Liste der einzubeziehenden Eigentümer Entitäten
  * OwnerId: Name/Id.
  * Owner-Share: Proportional share value

## Account allocation

In individual settlements, only a subset of accounts from the chart of
accounts are shown. Therefore, it is necessary to mark those
accounts which are to be to be used in a settlement.

The goal is a flexible definition of these settlement accounts. Unfortunately
the existing typification of the accounts in GnuCash is not sufficient for an
automated preselection of the accounts to be shown in groups.

Existing type:

* Assets
  * Bank accounts (Bank)
  * Cash accounts (Cash)
  * Credit Card Accounts (Credit Card)
  * Investment accounts (Mutual Fund)
  * Stock (Shares)
  * Stock trading (Trading)
* Liabilities
  * Accounts Receivable (Accounts Payable)
  * Accounts Payable (Accounts Receivable)

* Equity (Equity)
* Receivables
* Accounts Payable

* Expense
* Income

Therefore, the following proposal is choosen tp add new account types:

* Liabilities
  * Maintenance provisions

* Expense
  * Apportionable
  * Non apportionable
* Income
  * Apportionable
  * Non apportionable

This means that differentiation is also possible at a later date in
existing charts of accounts.

## Co-Owner billing reports

The aim of this new report is to provide an individual statement, that
provides co-owner bills for a given property.

## Property managment idioms

The following is an incomplete list of common idioms used for property managment.
The expressions should be considered in the translations:

Accounting
Accounting of the owner
Additional payment
Amount
Amounts that cannot be distributed
Contribution to maintenance reserve
Credit balance
Distribution key
Distributor total
Due date
Housing unit
Individual accounting
Name of the account
Name of property management
Owner share
Property
Property management
Provision for maintenance
Receipt
Recursive balance
Request
Settle owner
Settlement days
Settlement peak
Total administration costs
Total amount
Total settlement
Total result

Click the options button and select the owner you want to account for.
This owner statement is valid without signature
