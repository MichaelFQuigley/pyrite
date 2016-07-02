class testClass {
    char* testtt;
    public:
        void printSomething(char* something);
};

void testClass::printSomething(char* something)
{
   testtt = something; 
}

int main()
{
    testClass* tc = new testClass();
    tc->printSomething("test");
}
