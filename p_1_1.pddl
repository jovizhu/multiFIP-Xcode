(define (problem FR_1_1)
 (:domain first-response)
 (:objects  l1  - obj
	    f1 - obj
	    v1 - obj
	    m1 - obj
)
 (:init 	
 	(fire l1)     	
     	(water-at l1)
	(adjacent l1 l1)
	(fire-unit-at f1 l1)
 )
 (:goal (nfire l1))
 )
