(define (domain blocks-domain)
  (:types block)
  (:predicates 
		(on ?bm ?bf - block)
		(clear ?x - block)
		(on-table ?x - block)
		(emptyhand)
		(holding ?b - block)
		(not_tower)
)


(:action D_1_pick-up
  :parameters (?b1 ?b2 - block)
  :precondition (and (emptyhand) (clear ?b1) (on ?b1 ?b2))
  :effect (and (holding ?b1) (clear ?b2) (not (emptyhand)) (not (clear ?b1)) (not (on ?b1 ?b2)) (not_tower))
)
  

(:action D_2_pick-up
  :parameters (?b1 ?b2 - block)
  :precondition (and (emptyhand) (clear ?b1) (on ?b1 ?b2))
  :effect (and (on-table ?b1) (clear ?b2) (not (on ?b1 ?b2)) (not_tower))
)



  (:action D_1_put-on-block
      :parameters (?b1 ?b2 - block)
      :precondition (and (holding ?b1) (clear ?b2) (not_tower))
      :effect (and (on ?b1 ?b2) (emptyhand) (clear ?b1) (not (holding ?b1)) (not (clear ?b2)))                     
  )
  
  (:action D_2_put-on-block
        :parameters (?b1 ?b2 - block)
        :precondition (and (holding ?b1) (clear ?b2) (not_tower))
        :effect (and (on-table ?b1) (emptyhand) (clear ?b1) (not (holding ?b1)))
  )
  
  (:action put-down
      :parameters (?b - block)
      :precondition (and (holding ?b) (not_tower))
      :effect (and (on-table ?b) (emptyhand) (clear ?b) (not (holding ?b)))
  )

(:action D_1_pick-up-from-table
    :parameters (?b - block)
    :precondition (and (emptyhand) (clear ?b) (on-table ?b))
    :effect (and (emptyhand) (clear ?b) (on-table ?b))
  )
  
  
  (:action D_2_pick-up-from-table
      :parameters (?b - block)
      :precondition (and (emptyhand) (clear ?b) (on-table ?b))
      :effect (and (holding ?b) (not (emptyhand)) (not (on-table ?b)))
  )

  (:action D_1_pick-tower
    :parameters (?b1 ?b2 ?b3 - block)
    :precondition (and (emptyhand) (on ?b1 ?b2) (on ?b2 ?b3))
    :effect (and (emptyhand) (on ?b1 ?b2) (on ?b2 ?b3) (not (not_tower)))
  )
  
  (:action D_2_pick-tower
      :parameters (?b1 ?b2 ?b3 - block)
      :precondition (and (emptyhand) (on ?b1 ?b2) (on ?b2 ?b3))
      :effect (and (holding ?b2) (clear ?b3) (not (emptyhand)) (not (on ?b2 ?b3)) (not (not_tower)))
  )


  (:action D_1_put-tower-on-block
    :parameters (?b1 ?b2 ?b3 - block)
    :precondition (and (holding ?b2) (on ?b1 ?b2) (clear ?b3))
    :effect (and (on ?b2 ?b3) (emptyhand) (not (holding ?b2)) (not (clear ?b3)) (not_tower))            
  )
  
  
  (:action D_2_put-tower-on-block
      :parameters (?b1 ?b2 ?b3 - block)
      :precondition (and (holding ?b2) (on ?b1 ?b2) (clear ?b3))
      :effect (and (on-table ?b2) (emptyhand) (not (holding ?b2)) (not_tower))
  )



  (:action put-tower-down
    :parameters (?b1 ?b2 - block)
    :precondition (and (holding ?b2) (on ?b1 ?b2))
    :effect (and (on-table ?b2) (emptyhand) (not (holding ?b2)) (not_tower))
  )	

)