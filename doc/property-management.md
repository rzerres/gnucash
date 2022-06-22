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
