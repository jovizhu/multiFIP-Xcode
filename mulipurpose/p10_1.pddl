(define (problem bw_10_12)
	(:domain blocks-domain)

	(:action D_1_pick-up
	  :parameters (?b1 ?b2 - block)
	  :precondition (and (emptyhand) (clear ?b1) (on ?b1 ?b2))
          :effect (and (holding ?b1) (clear ?b2) (not (emptyhand)) (not (clear ?b1))     (not (on ?b1 ?b2)) )
	)
	
	(:goal (and (emptyhand) (on b1 b2) (on-table b2) (on b3 b5) (on-table b4)         (on-table b5) (clear b1) (clear b3) (clear b4)))
)

