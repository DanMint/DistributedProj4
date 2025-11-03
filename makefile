Build-Images:
	docker build -t prj5-bootstrap .
	docker build -t prj5-peer .
	docker build -t prj5-client .

Run-Testcase1:
	docker compose -f docker-compose-testcase-1.yml up

Run-Testcase2:
	docker compose -f docker-compose-testcase-2.yml up

Run-Testcase3:
	docker compose -f docker-compose-testcase-3.yml up

Run-Testcase4:
	docker compose -f docker-compose-testcase-4.yml up

Run-Testcase5:
	docker compose -f docker-compose-testcase-5.yml up
	