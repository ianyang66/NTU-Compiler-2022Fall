int main(){
    int b[3][4];
    int b1[1][1][2];
    b1[1][1][1] = 3;
    b[3][4] = 7;
    write(b1[1][1][1] + b[3][4]);
    write("\n");
    b1[1][1][2] = 4;
    b[2][3] = 5;
    write(b[2][3] - b1[1][1][2]);
    write("\n");
    return 0;
}
