--Create a grocery store database

CREATE TABLE clothingstore(id INTEGER PRIMARY KEY, type TEXT, price INTEGER, fabric TEXT, quantity INTEGER);

INSERT INTO clothingstore VALUES(1, 'jeans', 15.00, 'denim', 20);
SELECT * FROM clothingstore;
INSERT INTO clothingstore VALUES(2, 'blouses', 10.00, 'cotton', 25);
SELECT * FROM clothingstore;
INSERT INTO clothingstore VALUES(3, 'purses', 20.00, 'leather', 10);
SELECT * FROM clothingstore;
INSERT INTO clothingstore VALUES(4, 'shoes', 20.00, 'miscellaneous',10);
SELECT * FROM clothingstore;

--display the database ordered by price.
SELECT * FROM clothingstore 
ORDER BY price DESC;

--display items in the database with a price less than 20 and order them in ascending order.
SELECT * FROM clothingstore 
WHERE price < 20 ORDER BY price DESC;

--display the quantity of all of the items in the store.
SELECT SUM(quantity) FROM clothingstore;

