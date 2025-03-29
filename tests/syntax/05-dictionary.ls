func main() {
    // Dictionary literal
    a = {
        "aaa": "bbb",
        ccc: "ddd"
    };
    print(a.aaa);
    print(a["ccc"]);

    // Dictionary assignment
    dict = {};
    dict["aaa"] = "value1";
    dict["bbb"] = 123;

    // Dictionary remove
    for (k, v in dict) {
    	print("k = " + k + ", v = " + v);
    }
    print("unset");
    unset(dict, "aaa");
    for (k, v in dict) {
    	print("k = " + k + ", v = " + v);
    }
}
