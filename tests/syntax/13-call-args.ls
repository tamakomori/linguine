func main() {
    callee(123, 1.23, "abc");
}

func callee(a, b, c) {
    print("a=" + a);
    print("b=" + b);
    print("c=" + c);
}
