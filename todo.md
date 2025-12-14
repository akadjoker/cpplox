YNTAX SUGAR:
1. String Interpolation

var name = "Luis";
var age = 25;
print("Olá ${name}, tens ${age} anos!");

2. For-each loops
var nums = [1, 2, 3, 4, 5];
for (var n in nums) {
    print(n);
}

3. Structs / Classes
struct Point {
    x: 0,
    y: 0
}

var p = Point();
p.x = 10;
p.y = 20;
print(p.x + p.y);  // 30

2. Maps / Dictionaries
var person = {
    name: "Luis",
    age: 25,
    city: "Lisboa"
};

print(person["name"]);  // Luis
person["age"] = 26;

Arrays / Lists
var nums = [1, 2, 3, 4, 5];
nums[0] = 10;
print(nums[2]);  // 3

var matrix = [[1, 2], [3, 4]];
print(matrix[0][1]);  // 2