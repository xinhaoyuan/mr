(load "struct.def.ss")
(load "sort.ss")
(load "boot-vm.ss")
(define (debug . info) (display "DEBUG: ") (display info) (newline))

;; == COMPILER ================================================

;; All the compiler methods compile expressions into path. A path
;; contains several traces, including one trace for the entry and one
;; trace for the exit.

;; The main dispatcher
(define (compiler:compile-expression context expression)
  (expression:extract-context
   context expression
   (lambda (context expression)
     (cond

      ;; scope reference 
      ((expression:is-symbol? expression)
       (let ((binding (context:try-get-variable-binding
                       context
                       (expression:get-symbol expression)))
             (path  (path:create-empty))
             (trace (context:create-trace context)))
         (if binding
             (begin
               (trace:instruction-append!
                trace
                (instruction-load:@
                 (variable-binding:level binding)
                 (variable-binding:offset binding)
                 0
                 ))
               
               (trace:path-set!       trace path)
               (path:start-trace-set! path trace)
               (path:end-trace-set!   path trace)
               (path:regs-size-set!   path 1)
               
               path
               )
             ;; Binding not found, use symbol as global reference
             (begin
               (trace:instruction-append!
                trace
                (instruction-constant:@
                 (expression:get-symbol expression)
                 0))

               (trace:path-set!       trace path)
               (path:start-trace-set! path trace)
               (path:end-trace-set!   path trace)
               (path:regs-size-set!   path 1)

               path)
             )
         ))
      
      ;; constant number
      ((expression:is-number? expression)
       (let ((path  (path:create-empty))
             (trace (context:create-trace context)))
         (trace:instruction-append!
          trace
          (instruction-constant:@
           (expression:get-number expression)
           0))

         (trace:path-set!       trace path)
         (path:start-trace-set! path trace)
         (path:end-trace-set!   path trace)
         (path:regs-size-set!   path 1)

         path
         ))

      ;; constant string
      ((expression:is-string? expression)
       (let ((path  (path:create-empty))
             (trace (context:create-trace context)))
         (trace:instruction-append!
          trace
          (instruction-constant:@
           (expression:get-string expression)
           0))
         
         (trace:path-set!       trace path)
         (path:start-trace-set! path trace)
         (path:end-trace-set!   path trace)
         (path:regs-size-set!   path 1)

         path
         ))

      ;; check for syntax structure
      ((expression:is-list? expression)
       (let ((exp-list (expression:get-list expression)))
         (expression:extract-context
          context (car exp-list)
          (lambda (head-context head-expression)
            (cond

             ;; keyword
             ((expression:is-keyword? head-expression)
              (compiler:compile-keyword context
                                        (expression:get-symbol head-expression)
                                        (cdr exp-list)))

             ;; apply
             (else
              (compiler:compile-keyword context
                                        keyword:apply
                                        exp-list))
             ))
          )))
      
      ;; or we cannot handle it
      (else
       (context:report-error
        context
        "Unhandled expression to compile"))
      )
     )))

(define (compiler:compile-keyword context keyword exp-list)
  (cond

   ;; set! name value
   ((eq? keyword keyword:set!)
    (if (eq? (length exp-list) 2)
        (let ((name 
               (expression:extract-context
                context (car exp-list)
                (lambda (_ exp) exp)))
              )
          (if (expression:is-symbol? name)
              (let ((binding (context:try-get-variable-binding
                              context
                              (expression:get-symbol name)))
                    (value-path '()))
                (if binding
                    (begin
                      (set! value-path
                            (compiler:compile-expression context (car (cdr exp-list))))
                      (trace:instruction-append!
                       (path:end-trace value-path)
                       (instruction-store:@
                        (variable-binding:level binding)
                        (variable-binding:offset binding)
                        0))
                      ;; not to change the regs size
                      value-path
                      )
                    (context:report-error
                     context
                     "Cannot find the binding for set!")
                    )
                )
              (context:report-error
               context
               "Invalid variable name")))
        (context:report-error
         context
         "Invalid format for ``set!''")
        ))

   ;; if cond-exp then-branch else-branch
   ((eq? keyword keyword:if)
    (if (eq? (length exp-list) 3)
        (let ((cond-path (compiler:compile-expression
                          context (car exp-list)))
              (then-exp (car (cdr exp-list)))
              (else-exp (car (cdr (cdr exp-list))))
              )
          (let ((then-path
                 (compiler:compile-expression context then-exp))
                (else-path
                 (compiler:compile-expression context else-exp))
                (merge-trace (context:create-trace context)))

            (trace:instruction-append!
             (path:end-trace cond-path)
             (instruction-if:@ (path:start-trace then-path)
                               (path:start-trace else-path)
                               0))

            (trace:instruction-append!
             (path:end-trace then-path)
             (instruction-goto:@ merge-trace))

            (trace:instruction-append!
             (path:end-trace else-path)
             (instruction-goto:@ merge-trace))

            (trace:path-set!     merge-trace cond-path)
            (path:end-trace-set! cond-path merge-trace)
            (path:parent-set!    then-path cond-path)
            (path:parent-set!    else-path cond-path)
            (and (> (path:regs-size then-path)
                    (path:regs-size cond-path))
                 (path:regs-size-set!
                  cond-path
                  (path:regs-size then-path)))
            (and (> (path:regs-size else-path)
                    (path:regs-size cond-path))
                 (path:regs-size-set!
                  cond-path
                  (path:regs-size else-path)))
            cond-path
            ))
        (context:report-error
         context
         "Wrong number of branches in ``if''")
        ))

   ;; apply closure arg1 arg2 ...
   ((eq? keyword keyword:apply)
    (and (> (length exp-list) 0)
         (letrec ((path (path:create-empty))
                  (arg-path-vector
                   (make-vector (length exp-list) '()))
                  (eval-loop
                   (lambda (current index-list count)
                     (if (pair? current)
                         (let ((arg-path (compiler:compile-expression context (car current))))
                           (vector-set! arg-path-vector count arg-path)
                           (eval-loop (cdr current)
                                      (cons count index-list)
                                      (+ count 1)))
                         index-list
                         )))
                  (args-index-list '())
                  (apply-trace (context:create-trace context))
                  (concat-loop
                   (lambda (index-list save-count)
                     (and (pair? index-list)
                          (let ((current-path (vector-ref arg-path-vector (car index-list))))
                            
                            (path:offset-rel-set! current-path save-count)
                            (path:parent-set! current-path path)
                            (let ((cur-size (+ save-count
                                               (path:regs-size current-path))))
                              (and (> cur-size
                                      (path:regs-size path))
                                   (path:regs-size-set! path cur-size)))
                            (trace:instruction-append!
                             (path:end-trace current-path)
                             (instruction-goto:@
                              (if (pair? (cdr index-list))
                                  (path:start-trace
                                   (vector-ref arg-path-vector
                                               (car (cdr index-list))))
                                  apply-trace)))
                            
                            (concat-loop (cdr index-list) (+ save-count 1))
                            )
                         )))
                  (arg-push-loop
                   (lambda (index)
                     (if (< index (vector-length arg-path-vector))
                         (begin
                           (trace:instruction-append!
                            apply-trace
                            ((if (= index 0)
                                 instruction-apply-prepare:@
                                 instruction-apply-push-arg:@)
                             (path:offset-rel (vector-ref arg-path-vector index))))
                           (arg-push-loop (+ index 1)))
                         (trace:instruction-append!
                          apply-trace
                          (instruction-apply:@ 0)))))
                                                          
                  )
           ;; Sort the args path list by reversed order of register used
           (set! args-index-list
                 (merge-sort
                  (eval-loop exp-list '() 0)
                  (lambda (a b)
                    (> (path:regs-size (vector-ref arg-path-vector a))
                       (path:regs-size (vector-ref arg-path-vector b))))))
           (concat-loop args-index-list 0)
           (arg-push-loop 0)
           (path:start-trace-set!
            path
            (path:start-trace (vector-ref arg-path-vector (car args-index-list))))
           (path:end-trace-set!
            path
            apply-trace)
           (trace:path-set! apply-trace path)
           path
           )))

   ;; begin exp1 exp2 ... exptail
   ((eq? keyword keyword:begin)
    (if (eq? exp-list '())
        (context:report-error
         context
         "Begin list cannot be empty")
        (letrec ((path (compiler:compile-expression context (car exp-list)))
                 (eval
                  (lambda (current)
                    (and (pair? current)
                         (let ((current-path
                                (compiler:compile-expression context (car current))))
                           (path:parent-set! current-path path)
                           (and (> (path:regs-size current-path)
                                  (path:regs-size path))
                                (path:regs-size-set!
                                 path
                                 (path:regs-size current-path)))
                           (trace:instruction-append!
                            (path:end-trace path)
                            (instruction-goto:@ (path:start-trace current-path)))
                           (path:end-trace-set! path (path:end-trace current-path))
                           (eval (cdr current)))
                        ))))
          (eval (cdr exp-list))
          path
          )
        ))

   ;; lambda args-list ...
   ((eq? keyword keyword:lambda)
    (or
     (and (> (length exp-list) 1)
          (let ((args-list (expression:try-get-symbol-list (car exp-list)))
                (body-list (cdr exp-list))
                (entry '())
                (trace (context:create-trace context))
                (path (path:create-empty))
                )
            
            (or (and args-list
                     (begin
                       (context:push-scope-level! context 'variable args-list)
                       (set! entry
                             (context:gen-entry!
                              context
                              (compiler:compile-keyword
                               context keyword:begin body-list)))
                       (context:pop-scope-level! context)
                       (trace:instruction-append!
                        trace
                        (instruction-lambda:@
                         (length args-list)
                         (entry:entry-trace-id entry)
                         (entry:regs-size entry)
                         0))

                       (trace:path-set!       trace path)
                       (path:start-trace-set! path trace)
                       (path:end-trace-set!   path trace)
                       (path:regs-size-set!   path 1)

                       path
                       ))
                (context:report-error
                 context
                 "Invalid arguments list"))
            ))
     (context:report-error
      context
      "Invalid format for ``lambda''")
     ))

   ;; Unsupported keyword
   (else
    (context:report-error
     context "Unsupported keyword"))
   ))

;; == EXPRESSION ==============================================
(define keyword:if     'if)
(define keyword:lambda 'lambda)
(define keyword:set!   'set!)
(define keyword:begin  'begin)
(define keyword:apply  'apply)
(define (expression:extract-context context expression closure)
  (if (vector? expression)
      (closure (vector-ref expression 0)
               (vector-ref expression 1))
      (closure context expression)))
(define expression:is-symbol? symbol?)
(define expression:is-number? number?)
(define expression:is-string? string?)
(define expression:is-list?   pair?)
(define (expression:is-keyword? exp)
  (and (symbol? exp)
       (or (eq? exp keyword:if)
           (eq? exp keyword:lambda)
           (eq? exp keyword:set!)
           (eq? exp keyword:begin)
           (eq? exp keyword:apply)
           )))
(define (expression:get-symbol expression) expression)
(define (expression:get-string expression) expression)
(define (expression:get-number expression) expression)
(define (expression:get-list   expression) expression)
(define (expression:build-from-raw-sexp sexp) sexp)
(define (expression:try-get-symbol-list exp)
  (expression:extract-context
   '() exp
   (lambda (_ exp)
     (and (or (eq? exp '())
              (pair? exp))
          (letrec
              ((loop
                (lambda (current result)
                  (if (pair? current)
                      (let ((ele
                             (expression:extract-context
                              _ (car current)
                              (lambda (_ exp) exp))))
                        (and (symbol? ele)
                             (loop (cdr current) (cons ele result))))
                      result))))
            (reverse (loop exp '()))
            )))))

;; == CONTEXT =================================================
(define (context:create return-cc)
  (context:@ return-cc '() '() 0))
(define (context:report-error context message)
  (display message) (newline)
  ((context:return-continuation context) '())
  )
(define (context:create-trace context)
  (trace:@ #f '() '() context '())
  )
(define (context:try-get-variable-binding context name)
  (letrec ((find-level
            (lambda (level-list depth)
              (if (pair? level-list)
                  (if (eq? (car (car level-list)) 'variable)
                      (or (find-offset (cdr (car level-list)) depth 0)
                          (find-level (cdr level-list) (+ depth 1)))
                      (find-level (cdr level-list) depth))
                  #f)))
           (find-offset
            (lambda (level depth offset)
              (if (pair? level)
                  (if (eq? (car level) name)
                      (variable-binding:@ depth offset)
                      (find-offset (cdr level) depth (+ offset 1)))
                  #f)))
           )
    (find-level (context:scope-level-list context) 0)))
(define (context:push-scope-level! context type list)
  (context:scope-level-list-set!
   context
   (cons (cons type list)
         (context:scope-level-list context))))
(define (context:pop-scope-level! context)
  (context:scope-level-list-set!
   context
   (cdr (context:scope-level-list context))))

(define (context:gen-entry! context path)
  (trace:instruction-append!
   (path:end-trace path)
   (instruction-return:@
    (path:offset-rel path)))
  (letrec
      ((trace-list (context:trace-list context))
       (trace-count (context:trace-count context))
       (scan-path
        (lambda (path)
          (and (eq? (path:offset-abs path) '())
               (if (eq? (path:parent path) '())
                   (path:offset-abs-set!
                    path
                    (path:offset-rel path))
                   (begin
                     (scan-path (path:parent path))
                     (path:offset-abs-set!
                      path
                      (+ (path:offset-abs (path:parent path))
                         (path:offset-rel path))))))
          ))
       (scan-trace
        (lambda (trace)
          (or (trace:scan-flag trace)
              (let ((last-instruction (car (trace:instruction-list trace)))
                    (assign-scan-id
                     (lambda ()
                       (trace:scan-id-set! trace trace-count)
                       (set! trace-count (+ trace-count 1))
                       (set! trace-list (cons trace trace-list))))
                    (set-stub
                     (lambda ()
                       (trace:scan-id-set! trace '())))
                    )
                (trace:scan-flag-set! trace #t)
                (scan-path (trace:path trace))
                (cond
                 ((instruction-if:? last-instruction)
                  (let ((then-trace (instruction-if:then-branch last-instruction))
                        (else-trace (instruction-if:else-branch last-instruction))
                        )
                    (scan-trace then-trace)
                    (scan-trace else-trace)
                    (assign-scan-id)
                    (instruction-if:then-branch-set!
                     last-instruction
                     (trace:scan-id then-trace))
                    (instruction-if:else-branch-set!
                     last-instruction
                     (trace:scan-id else-trace))
                    ))
                 ((instruction-goto:? last-instruction)
                  (let ((target-trace (instruction-goto:target last-instruction))
                        )
                    (scan-trace target-trace)
                    (if (eq? (trace:scan-id target-trace) '())
                        (if (eq? (cdr (trace:instruction-list trace)) '())
                            (set-stub)
                            (let ((last-eff-instruction
                                   (car (cdr (trace:instruction-list trace)))))
                              (assign-scan-id)
                              (and (instruction-apply:? last-eff-instruction)
                                   (begin
                                     (trace:instruction-list-set!
                                      trace
                                      (cdr (cdr (trace:instruction-list trace))))
                                     (trace:instruction-append!
                                      trace
                                      (instruction-apply-tail:@))))))
                        (assign-scan-id)
                        )
                    (instruction-goto:target-set!
                     last-instruction
                     (trace:scan-id target-trace))
                    ))
                 ((instruction-return:? last-instruction)
                  (if (eq? (cdr (trace:instruction-list trace)) '())
                      (set-stub)
                      (let ((last-eff-instruction
                             (car (cdr (trace:instruction-list trace)))))
                        (assign-scan-id)
                        (and (instruction-apply:? last-eff-instruction)
                             (begin
                               (trace:instruction-list-set!
                                trace
                                (cdr (cdr (trace:instruction-list trace))))
                               (trace:instruction-append!
                                trace
                                (instruction-apply-tail:@)))))
                      ))
                 (else
                  (debug last-instruction)
                  (context:report-error
                   context
                   "Unknow last instruction in a trace"))
                 ))
              )))
       )
    (scan-trace (path:start-trace path))
    (context:trace-list-set! context trace-list)
    (context:trace-count-set! context trace-count)
    (entry:@ (trace:scan-id (path:start-trace path))
             (path:regs-size path))
    ))

(define (context:solidify context entry)
  (letrec ((trace-vector (make-vector (context:trace-count context) '()))
           (fill
            (lambda (current count)
              (and (pair? current)
                   (begin
                     (vector-set!
                      trace-vector count
                      (list->vector (trace:dump-instruction-list (car current))))
                     (fill (cdr current) (- count 1))))))
           )
    (fill (context:trace-list context) (- (context:trace-count context) 1))
    (cons trace-vector (vector (entry:entry-trace-id entry)
                               (entry:regs-size entry)))
    ))

;; == INSTRUCTION =============================================
(define instruction-head:constant       'constant)
(define instruction-head:if             'if)
(define instruction-head:goto           'goto)
(define instruction-head:return         'return)
(define instruction-head:apply-prepare  'apply-prepare)
(define instruction-head:apply-push-arg 'apply-push-arg)
(define instruction-head:apply          'apply)
(define instruction-head:apply-head     'apply-head)
(define instruction-head:load           'load)
(define instruction-head:store          'store)
(define instruction-head:lambda         'lambda)

;; == TRACE ===================================================
(define (trace:instruction-append! trace instruction)
  (trace:instruction-list-set!
   trace
   (cons instruction (trace:instruction-list trace))))
(define (trace:dump-instruction-list trace)
  (letrec
      ((scan
        (lambda (current result)
          (if (pair? current)
              (scan
               (cdr current)
               (cons
                (let ((origin (car current)))
                  (cond
                   ((instruction-constant:? origin)
                    (vector instruction-head:constant
                            (instruction-constant:constant origin)
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-constant:dst-reg-id origin))
                            )
                    )
                   ((instruction-if:? origin)
                    (vector instruction-head:if 
                            (instruction-if:then-branch origin)
                            (instruction-if:else-branch origin)
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-if:cond-reg-id origin))
                            )
                    )
                   ((instruction-goto:? origin)
                    (vector instruction-head:goto
                            (instruction-goto:target origin))
                    )
                   ((instruction-return:? origin)
                    (vector instruction-head:return
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-return:src origin))
                            )
                    )
                   ((instruction-apply-prepare:? origin)
                    (vector instruction-head:apply-prepare
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-apply-prepare:src-reg-id origin))
                            )
                    )
                   ((instruction-apply-push-arg:? origin)
                    (vector instruction-head:apply-push-arg
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-apply-push-arg:src-reg-id origin))
                            )
                    )
                   ((instruction-apply:? origin)
                    (vector instruction-head:apply
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-apply:dst-reg-id origin))
                            )
                    )
                   ((instruction-apply-tail:? origin)
                    (vector instruction-head:apply-tail)
                    )
                   ((instruction-load:? origin)
                    (vector instruction-head:load  
                            (instruction-load:level origin)
                            (instruction-load:offset origin)
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-load:dst-reg-id origin))
                            )
                    )
                   ((instruction-store:? origin)
                    (vector instruction-head:store
                            (instruction-store:level origin)
                            (instruction-store:offset origin)
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-store:src-reg-id origin))
                            )
                    )
                   ((instruction-lambda:? origin)
                    (vector instruction-head:lambda
                            (instruction-lambda:args-count origin)
                            (instruction-lambda:entry-id origin)
                            (instruction-lambda:regs-size origin)
                            (+ (path:offset-abs (trace:path trace))
                               (instruction-lambda:dst-reg-id origin))
                            )
                    )
                   ))
                result))
              result)))
       )
    (scan (trace:instruction-list trace) '())))

(define (path:create-empty)
  (path:@ '() ;; start-trace
          '() ;; end-trace
          0   ;; regs-size
          '() ;; offset-abs
          0   ;; offset-rel
          '() ;; parent
          ))

;; == SHELL ===================================================
(define (shell:compile-raw-sexp input)
  (call-with-current-continuation
   (lambda (cc)
     (let ((context (context:create cc))
           (expression (expression:build-from-raw-sexp input))
           )       
       (context:solidify
        context
        (context:gen-entry!
         context
         (compiler:compile-expression context expression)))
       )
     )))
(define (shell:repl)
  (let ((prog '()))
    (display "repl> ")
    (set! prog (read))
    (or (eof-object? prog)
        (begin
          (set! prog (shell:compile-raw-sexp prog))
          (display (vm:execute-prog prog)) (newline)
          (shell:repl)))
    ))

;; == UNIT-TEST ===============================================

;; constants
(define shell:unit-case-1 46)
;; if
(define shell:unit-case-2
  '(if 1 (if 2 3 4) 5))
;; apply
(define shell:unit-case-3
  '(1 (10 11 12 13 14) (if 2 (3 4 5) 6) 7 8))
;; apply and if
(define shell:unit-case-4
  '(1 (if 2 3 4) 5 6))
;; begin
(define shell:unit-case-5
  '(begin 1 (if 2 3 4) 5 6))
;; lambda, binding and set!
(define shell:unit-case-6
  '(lambda (a1 a2 a3) 1 2 3 (lambda (b) (set! a3 b)))
  )
;; tail apply
(define shell:unit-case-7
  '((lambda (loop)
      (set! loop (lambda (x)
                   (if (< x 1000000)
                       (loop (+ x 1))
                       (print x))))
      (loop 0)) 0)
  )

(define (shell:unit-test)
  (let ((prog (shell:compile-raw-sexp shell:unit-case-7)))
    (vm:execute-prog prog))
  )
