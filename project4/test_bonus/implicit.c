int f(float a) {
    return a;
}
float n(int a) {
    return a;
}

int main() {
    int a;
    float b;

    a = f(1.4);
    write(a);
    write("\n");

    write(n(1));
    write("\n");

    return 0;
}
