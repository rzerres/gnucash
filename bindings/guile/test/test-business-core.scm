(use-modules (srfi srfi-1))
(use-modules (srfi srfi-64))
(use-modules (gnucash app-utils))
(use-modules (tests srfi64-extras))
(use-modules (tests test-engine-extras))
(use-modules (gnucash utilities))
(use-modules (gnucash engine))

(define (run-test)
  (test-runner-factory gnc:test-runner)
  (test-begin "test-business-core")
  (core-tests-coowner)
  (core-tests-customer)
  (test-end "test-business-core"))

(define (get-currency sym)
  (gnc-commodity-table-lookup
   (gnc-commodity-table-get-table (gnc-get-current-book))
   (gnc-commodity-get-namespace (gnc-default-report-currency))
   sym))

(define structure
  (list "Root" (list (cons 'type ACCT-TYPE-ASSET)
                     (cons 'commodity (get-currency "USD")))
        (list "Asset"
              (list "Bank-GBP" (list (cons 'commodity (get-currency "GBP"))))
              (list "Bank-EUR" (list (cons 'commodity (get-currency "EUR"))))
              (list "Bank-USD"))
        (list "VAT"
              (list "VAT-on-Purchases-10")
              (list "VAT-on-Purchases-19" (list (cons 'commodity (get-currency "EUR"))))
              (list "VAT-on-Sales-10" (list (cons 'type ACCT-TYPE-LIABILITY)))
              (list "VAT-on-Sales-19" (list (cons 'type ACCT-TYPE-LIABILITY)
                                            (cons 'commodity (get-currency "EUR")))))
        (list "Income" (list (cons 'type ACCT-TYPE-INCOME))
              (list "Income-USD")
              (list "Income-GBP" (list (cons 'commodity (get-currency "GBP"))))
              (list "Income-EUR" (list (cons 'commodity (get-currency "EUR")))))
        (list "A/Receivable" (list (cons 'type ACCT-TYPE-RECEIVABLE))
              (list "AR-USD")
              (list "AR-GBP" (list (cons 'commodity (get-currency "GBP"))))
              (list "AR-EUR" (list (cons 'commodity (get-currency "EUR")))))
        (list "A/Payable" (list (cons 'type ACCT-TYPE-PAYABLE))
              (list "AP-USD")
              (list "AP-GBP" (list (cons 'commodity (get-currency "GBP"))))
              (list "AP-EUR" (list (cons 'commodity (get-currency "EUR")))))
   ))

;; Testing entity Co-Owner
(define (core-tests-coowner)
  (let* ((env (create-test-env))
         ;; create the account hirarchy from given structure definition
         (account-alist (env-create-account-structure-alist env structure))
         (get-acct (lambda (name)
                     (or (assoc-ref account-alist name)
                     (error "invalid account name" name))))
         (YEAR (gnc:time64-get-year (gnc:get-today)))

         (coowner-1 (let ((coowner-1 (gncCoOwnerCreate (gnc-get-current-book))))
                (gncCoOwnerSetID coowner-1 "coowner-1-id")
                (gncCoOwnerSetName coowner-1 "coowner-1-name")
                (gncCoOwnerSetNotes coowner-1 "coowner-1-notes")
                (gncCoOwnerSetCurrency coowner-1 (get-currency "EUR"))
                (gncCoOwnerSetTaxIncluded coowner-1 1) ; 1 = GNC-TAXINCLUDED-YES
                coowner-1))

         (owner-1 (let ((owner-1 (gncOwnerNew)))
                 (gncOwnerInitCoOwner owner-1 coowner-1)
                    owner-1))

         ;; job-1 is generated for a Co-Owner
         (job-1 (let ((job-1 (gncJobCreate (gnc-get-current-book))))
                  (gncJobSetID job-1 "job-1-id")
                  (gncJobSetName job-1 "job-1-name")
                  (gncJobSetOwner job-1 owner-1)
                  job-1))

         ;; inv-1 is generated for a Co-Owner
         (inv-1 (let ((inv-1 (gncInvoiceCreate (gnc-get-current-book))))
                  (gncInvoiceSetOwner inv-1 owner-1)
                  (gncInvoiceSetNotes inv-1 "inv-1-notes")
                  (gncInvoiceSetBillingID inv-1 "inv-1-billing-id")
                  (gncInvoiceSetCurrency inv-1 (get-currency "EUR"))
                  inv-1))

         ;; entry will generate an income amount in Euro to curren book
         (entry (lambda (amt)
                  (let ((entry (gncEntryCreate (gnc-get-current-book))))
                    (gncEntrySetDateGDate entry (time64-to-gdate (current-time)))
                    (gncEntrySetDescription entry "entry-1-desc")
                    (gncEntrySetAction entry "entry-1-action")
                    (gncEntrySetNotes entry "entry-1-notes")
                    (gncEntrySetInvAccount entry (get-acct "Income-EUR"))
                    (gncEntrySetDocQuantity entry 1 #f)
                    (gncEntrySetInvPrice entry amt)
                    entry)))


         ;; entry-1  1 widgets of $15 = $15
         (entry-1 (entry 15))

         (standard-vat-sales-tt
          (let ((tt (gncTaxTableCreate (gnc-get-current-book))))
            (gncTaxTableIncRef tt)
            (gncTaxTableSetName tt "19% vat on sales")
            (let ((entry (gncTaxTableEntryCreate)))
              (gncTaxTableEntrySetAccount entry (get-acct "VAT-on-Sales-19"))
              (gncTaxTableEntrySetType entry GNC-AMT-TYPE-PERCENT)
              (gncTaxTableEntrySetAmount entry 19)
              (gncTaxTableAddEntry tt entry))
            tt))

         (standard-vat-purchases-tt
          (let ((tt (gncTaxTableCreate (gnc-get-current-book))))
            (gncTaxTableIncRef tt)
            (gncTaxTableSetName tt "19% vat on purchases")
            (let ((entry (gncTaxTableEntryCreate)))
              (gncTaxTableEntrySetAccount entry (get-acct "VAT-on-Purchases-19"))
              (gncTaxTableEntrySetType entry GNC-AMT-TYPE-PERCENT)
              (gncTaxTableEntrySetAmount entry 19)
              (gncTaxTableAddEntry tt entry))
            tt)))


    ;; inv-1 €15, due 15.1.2022 after report-date i.e. "current"
    (let ((inv-1-copy (gncInvoiceCopy inv-1)))
      (gncInvoiceAddEntry inv-1-copy (entry 27/4))
      (gncInvoicePostToAccount inv-1-copy
                               (get-acct "AR-EUR")         ;post-to acc
                               (gnc-dmy2time64 10 01 2022) ;posted
                               (gnc-dmy2time64 25 01 2022) ;due
                               "inv current €15.00" #t #f)
      (gncInvoiceApplyPayment
       inv-1-copy '() (get-acct "Bank-EUR") 15 1
       (gnc-dmy2time64 24 01 2022)
       "inv €15" "fully paid"))

    ;; check Co-Owner structure attributes
    (test-equal "gnc:owner-get-name-dep"
      "coowner-1-name"
      (gnc:owner-get-name-dep owner-1))

    (test-equal "gnc:owner-get-address-dep"
      ""
      (gnc:owner-get-address-dep owner-1))

    (test-equal "gnc:owner-get-name-and-address-dep"
      "coowner-1-name\n"
      (gnc:owner-get-name-and-address-dep owner-1))

    (test-equal "gnc:owner-get-owner-id"
      "coowner-1-id"
      (gnc:owner-get-owner-id owner-1))

    ;; Check Co-Owner attributes in splits
    (let ((new-owner (gncOwnerNew)))

      (test-equal "new-owner is initially empty"
        ""
        (gncOwnerGetName new-owner))

      ;; asure split->owner hashtable is empty at start
      (gnc:split->owner #f)

      (test-equal "gnc:split->owner (from AR) return"
        #f
        (gncOwnerGetName
          (gnc:split->owner
            (last (xaccAccountGetSplitList (get-acct "AR-EUR"))))
          new-owner)))
    ))

;; Testing entity Customer
(define (core-tests-customer)
  (let* ((env (create-test-env))
         (account-alist (env-create-account-structure-alist env structure))
         (get-acct (lambda (name)
                     (or (assoc-ref account-alist name)
                     (error "invalid account name" name))))
         (YEAR (gnc:time64-get-year (gnc:get-today)))

         ;; customer tests
         (cust-1 (let ((cust-1 (gncCustomerCreate (gnc-get-current-book))))
                   (gncCustomerSetID cust-1 "cust-1-id")
                   (gncCustomerSetName cust-1 "cust-1-name")
                   (gncCustomerSetNotes cust-1 "cust-1-notes")
                   (gncCustomerSetCurrency cust-1 (get-currency "USD"))
                   (gncCustomerSetTaxIncluded cust-1 1) ;1 = GNC-TAXINCLUDED-YES
                   cust-1))

         (cust-2 (let ((cust-2 (gncCustomerCreate (gnc-get-current-book))))
                   (gncCustomerSetID cust-2 "cust-2-id")
                   (gncCustomerSetName cust-2 "cust-2-name")
                   (gncCustomerSetNotes cust-2 "cust-2-notes")
                   (gncCustomerSetCurrency cust-2 (get-currency "USD"))
                   (gncCustomerSetTaxIncluded cust-2 1) ;1 = GNC-TAXINCLUDED-YES
                   cust-2))

         (owner-1 (let ((owner-1 (gncOwnerNew)))
                    (gncOwnerInitCustomer owner-1 cust-1)
                    owner-1))

         (owner-2 (let ((owner-2 (gncOwnerNew)))
                    (gncOwnerInitCustomer owner-2 cust-2)
                    owner-2))

         ;; job-1 is generated for a customer
         (job-1 (let ((job-1 (gncJobCreate (gnc-get-current-book))))
                  (gncJobSetID job-1 "job-1-id")
                  (gncJobSetName job-1 "job-1-name")
                  (gncJobSetOwner job-1 owner-1)
                  job-1))

         ;; inv-1 is generated for a customer
         (inv-1 (let ((inv-1 (gncInvoiceCreate (gnc-get-current-book))))
                  (gncInvoiceSetOwner inv-1 owner-1)
                  (gncInvoiceSetNotes inv-1 "inv-1-notes")
                  (gncInvoiceSetBillingID inv-1 "inv-1-billing-id")
                  (gncInvoiceSetCurrency inv-1 (get-currency "USD"))
                  inv-1))

         ;; inv-2 is generated for a customer
         (inv-2 (let ((inv-2 (gncInvoiceCreate (gnc-get-current-book))))
                  (gncInvoiceSetOwner inv-2 owner-2)
                  (gncInvoiceSetNotes inv-2 "inv-2-notes")
                  (gncInvoiceSetCurrency inv-2 (get-currency "USD"))
                  inv-2))

         (entry (lambda (amt)
                  (let ((entry (gncEntryCreate (gnc-get-current-book))))
                    (gncEntrySetDateGDate entry (time64-to-gdate (current-time)))
                    (gncEntrySetDescription entry "entry-desc")
                    (gncEntrySetAction entry "entry-action")
                    (gncEntrySetNotes entry "entry-notes")
                    (gncEntrySetInvAccount entry (get-acct "Income-USD"))
                    (gncEntrySetDocQuantity entry 1 #f)
                    (gncEntrySetInvPrice entry amt)
                    entry)))

         ;; entry-1  1 widgets of $6 = $6
         (entry-1 (entry 6))

         ;; entry-2  3 widgets of EUR4 = EUR12
         (entry-2 (let ((entry-2 (gncEntryCreate (gnc-get-current-book))))
                    (gncEntrySetDateGDate entry-2 (time64-to-gdate (current-time)))
                    (gncEntrySetDescription entry-2 "entry-2-desc")
                    (gncEntrySetAction entry-2 "entry-2-action")
                    (gncEntrySetNotes entry-2 "entry-2-notes")
                    (gncEntrySetInvAccount entry-2 (get-acct "Income-EUR"))
                    (gncEntrySetDocQuantity entry-2 3 #f)
                    (gncEntrySetInvPrice entry-2 4)
                    entry-2))

         ;; entry-3  5 widgets of GBP7 = GBP35
         (entry-3 (let ((entry-3 (gncEntryCreate (gnc-get-current-book))))
                    (gncEntrySetDateGDate entry-3 (time64-to-gdate (current-time)))
                    (gncEntrySetDescription entry-3 "entry-3-desc")
                    (gncEntrySetAction entry-3 "entry-3-action")
                    (gncEntrySetNotes entry-3 "entry-3-notes")
                    (gncEntrySetInvAccount entry-3 (get-acct "Income-GBP"))
                    (gncEntrySetDocQuantity entry-3 5 #f)
                    (gncEntrySetInvPrice entry-3 7)
                    entry-3))

         (standard-vat-sales-tt
          (let ((tt (gncTaxTableCreate (gnc-get-current-book))))
            (gncTaxTableIncRef tt)
            (gncTaxTableSetName tt "10% vat on sales")
            (let ((entry (gncTaxTableEntryCreate)))
              (gncTaxTableEntrySetAccount entry (get-acct "VAT-on-Sales-10"))
              (gncTaxTableEntrySetType entry GNC-AMT-TYPE-PERCENT)
              (gncTaxTableEntrySetAmount entry 10)
              (gncTaxTableAddEntry tt entry))
            tt))

         (standard-vat-purchases-tt
          (let ((tt (gncTaxTableCreate (gnc-get-current-book))))
            (gncTaxTableIncRef tt)
            (gncTaxTableSetName tt "10% vat on purchases")
            (let ((entry (gncTaxTableEntryCreate)))
              (gncTaxTableEntrySetAccount entry (get-acct "VAT-on-Purchases-10"))
              (gncTaxTableEntrySetType entry GNC-AMT-TYPE-PERCENT)
              (gncTaxTableEntrySetAmount entry 10)
              (gncTaxTableAddEntry tt entry))
            tt)))

    ;; inv-1 €15, due 15.1.2022 after report-date i.e. "current"
    (let ((inv-1-copy (gncInvoiceCopy inv-1)))
      (gncInvoiceAddEntry inv-1-copy (entry 27/4))
      (gncInvoicePostToAccount inv-1-copy
                               (get-acct "AR-EUR")         ;post-to acc
                               (gnc-dmy2time64 10 01 2022) ;posted
                               (gnc-dmy2time64 15 01 2022) ;due
                               "inv current €15.00" #t #f)
      (gncInvoiceApplyPayment
       inv-1-copy '() (get-acct "Bank-EUR") 15 1
       (gnc-dmy2time64 30 01 2022)
       "inv €15" "fully paid"))

    (test-equal "gnc:owner-get-name-dep"
      "cust-1-name"
      (gnc:owner-get-name-dep owner-1))

    (test-equal "gnc:owner-get-address-dep"
      ""
      (gnc:owner-get-address-dep owner-1))

    ;; inv-1 $6, due 18.7.1980 after report-date i.e. "current"
    (let ((inv-1-copy (gncInvoiceCopy inv-1)))
      (gncInvoiceAddEntry inv-1-copy (entry 27/4))
      (gncInvoicePostToAccount inv-1-copy
                               (get-acct "AR-USD")         ;post-to acc
                               (gnc-dmy2time64 13 05 1980) ;posted
                               (gnc-dmy2time64 18 07 1980) ;due
                               "inv current $6.75" #t #f)
      (gncInvoiceApplyPayment
       inv-1-copy '() (get-acct "Bank-USD") 3/2 1
       (gnc-dmy2time64 18 03 1980)
       "inv >90 payment" "pay only $1.50")
      (gncInvoiceApplyPayment
       inv-1-copy '() (get-acct "Bank-USD") 2 1
       (gnc-dmy2time64 20 03 1980)
       "inv >90 payment" "pay only $2.00"))

    ;; inv-1-copy due 18.6.1980, <30days before report date
    ;; amount due $12
    (let ((inv-1-copy (gncInvoiceCopy inv-1)))
      (gncInvoiceAddEntry inv-1-copy (entry 4))
      (gncInvoicePostToAccount inv-1-copy
                               (get-acct "AR-USD")         ;post-to acc
                               (gnc-dmy2time64 13 04 1980) ;posted
                               (gnc-dmy2time64 18 06 1980) ;due
                               "inv <30days $4.00" #t #f))

    ;; inv-1-copy due 18.5.1980, 30-60days before report date
    ;; amount due $6
    (let ((inv-1-copy (gncInvoiceCopy inv-1)))
      (gncInvoiceAddEntry inv-1-copy (entry 17/2))
      (gncInvoicePostToAccount inv-1-copy
                               (get-acct "AR-USD")         ;post-to acc
                               (gnc-dmy2time64 13 03 1980) ;posted
                               (gnc-dmy2time64 18 05 1980) ;due
                               "inv 30-60 $8.50" #t #f))

    ;; inv-1-copy due 18.4.1980, 60-90days before report date
    ;; amount due $6
    (let ((inv-1-copy (gncInvoiceCopy inv-1)))
      (gncInvoiceAddEntry inv-1-copy (entry 15/2))
      (gncInvoicePostToAccount inv-1-copy
                               (get-acct "AR-USD")         ;post-to acc
                               (gnc-dmy2time64 13 02 1980) ;posted
                               (gnc-dmy2time64 18 04 1980) ;due
                               "inv 60-90 $7.50" #t #f))

    ;; inv-1-copy due 18.3.1980, >90days before report date
    ;; amount due $11.50, drip-payments
    (let ((inv-1-copy (gncInvoiceCopy inv-1)))
      (gncInvoiceAddEntry inv-1-copy (entry 23/2))
      (gncInvoicePostToAccount inv-1-copy
                               (get-acct "AR-USD")         ;post-to acc
                               (gnc-dmy2time64 13 01 1980) ;posted
                               (gnc-dmy2time64 18 03 1980) ;due
                               "inv >90 $11.50" #t #f)
      (gncInvoiceApplyPayment
       inv-1-copy '() (get-acct "Bank-USD") 3/2 1
       (gnc-dmy2time64 18 03 1980)
       "inv >90 payment" "pay only $1.50")
      (gncInvoiceApplyPayment
       inv-1-copy '() (get-acct "Bank-USD") 2 1
       (gnc-dmy2time64 20 03 1980)
       "inv >90 payment" "pay only $2.00"))

    ;; inv-2-copy due 18.4.1980, >90days before report date
    ;; amount due $11.50, drip-payments
    (let ((inv-2-copy (gncInvoiceCopy inv-2)))
      (gncInvoiceAddEntry inv-2-copy (entry 200))
      (gncInvoicePostToAccount inv-2-copy
                               (get-acct "AR-USD")         ;post-to acc
                               (gnc-dmy2time64 18 04 1980) ;posted
                               (gnc-dmy2time64 18 04 1980) ;due
                               "inv $200" #t #f)
      (gncInvoiceApplyPayment
       inv-2-copy '() (get-acct "Bank-USD") 200 1
       (gnc-dmy2time64 19 04 1980)
       "inv $200" "fully paid"))

    (let ((inv-2-copy (gncInvoiceCopy inv-2)))
      (gncInvoiceAddEntry inv-2-copy (entry -5/2))
      (gncInvoiceSetIsCreditNote inv-2-copy #t)
      (gncInvoicePostToAccount inv-2-copy
                               (get-acct "AR-USD")         ;post-to acc
                               (gnc-dmy2time64 22 06 1980) ;posted
                               (gnc-dmy2time64 22 06 1980) ;due
                               "inv $2.50 CN" #t #f))

    (test-equal "gnc:owner-get-name-dep"
      "cust-1-name"
      (gnc:owner-get-name-dep owner-1))

    (test-equal "gnc:owner-get-address-dep"
      ""
      (gnc:owner-get-address-dep owner-1))

    (test-equal "gnc:owner-get-name-and-address-dep"
      "cust-1-name\n"
      (gnc:owner-get-name-and-address-dep owner-1))

    (test-equal "gnc:owner-get-owner-id"
      "cust-1-id"
      (gnc:owner-get-owner-id owner-1))

    ;; a non-business transaction
    (env-transfer env 01 01 1990
                  (get-acct "Income-GBP") (get-acct "Bank-GBP") 10)

    ))
