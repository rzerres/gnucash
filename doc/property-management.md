# Property management

Gnucash may help to ease the management of your property. If you use it
for your own house or apartment, it already offers all tasks assigned
to the bookkeeping part. To handle multiple apartments or rentals in
a community property you have to get hold of some other tasks.

## Background

In Germany, a co-owners' association (WEG) is free to decide to
self-manage their property in an owner administration. Tasks of
[property][property_mgmt] management can then be taken over by an
owner of the WEG appointed for this purpose.

The rules may be adopted to the needs in other countries as well.

[property_mgmt]: https://www.hausverwaltung-ratgeber.de/hausverwaltung.html).

## Implementation

Following the implementation of customers and suppliers entities, a `Co-Owner`
entity has been introduced. This one is used to offer functions to

* Create
* Edit
* List
* Search

this new entity type.

Much like the implemented of other business features, you may assign

* Invoices
* Orders
* Payments
* Settlements

to `Co-Owner` entities. They offer the storage of attributes, unique
to a given `Co-Owner` and probably vary per housing unit. Property
management tasks will consume those attributes.

For example, the following object identifiers must be assigned to
create correct settlements:

* the property shares
* the apartment unit
* a distribution key

With regard to accounting tasks a suitable account structure is
required. As usual you will assign the booking ledgers inside the accounts in charge.

The legal regulations that exist in Germany stipulate, that individual
settlements are created annually for each Co-Owner. In particular
this settlements must show and proportionately allocate

* apportion able amounts
* non-apportion able amounts
* incomes
* individual costs

as well as a report of the

* property maintenance reserve

## Distribution lists

The introduction of `distribution lists` allow the definition of
parameters that are required for the calculation of settlement values.
The given values will be consumed in reports, calculating the
respective settlement amounts for the given co-owner.

Take advantage of `types` inside each `distribution list` object. You
are able to assign a hierarchy of accounts to a given `distribution
list`. The settlement calculation will apply the apportion-able
amounts inside the reports.

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
  * OwnerId: Name/Id.
  * Owner-Share: Proportional share value

## Account allocation

In individual settlements, only a subset of accounts from the chart of
accounts are shown. Therefore, it is necessary to mark those
accounts which are to be used in a settlement.

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

Therefore, the following proposal is choosen to add new account types:

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
