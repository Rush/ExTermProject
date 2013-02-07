#include <QCoreApplication>
#include <QLinkedList>
#include <QDebug>

struct StyleEntry {
    int id;
    StyleEntry() {
        id = -1;
    }

    StyleEntry(int _id) {
        id = _id;
    }
    quint16 from, to;

    StyleEntry(quint16 _from, quint16 _to, int _id) {
        from = _from;
        to =_to;
        id = _id;
    }

    friend QDebug& operator << (QDebug debug, const StyleEntry& myObj);
};

Q_DECLARE_METATYPE(StyleEntry)

inline QDebug& operator << (QDebug debug, const StyleEntry &entry) {
    debug << "(id=" << entry.id << "|" <<  entry.from << "," << entry.to << ")";
    return debug.space();
}

typedef QLinkedList<StyleEntry> StyleEntryList;

StyleEntryList styles;

StyleEntryList::iterator iteratorFor(int from, int to, int id) {
    auto i = styles.begin();
    auto entry = StyleEntry(id);

    auto end = styles.end();
    StyleEntryList::iterator fromI = end, toI = end;

    // find element from which we will need to check and/or remove
    for(;i != styles.end();++i) {
        if(i->from < from) {
            fromI = StyleEntryList::iterator(i);
        }
    }
    for(i=styles.begin();i != styles.end();++i) {
        if(i->to > to) {
            toI = StyleEntryList::iterator(i);
            break;
        }
    }

    // override all elements or insert into empty list
    if(fromI == end && toI == end) {
        styles.clear();
        return styles.insert(styles.end(), entry);
    }
    // insert inside element
    else if(fromI == toI) {
        StyleEntry copy = *toI;
        fromI->to = from-1;
        copy.from = to+1;
        auto second = styles.insert(fromI+1, copy);
        auto res = styles.insert(second, entry);
        return res;
    }
    else {
        if(fromI != styles.end())
            fromI->to = qMin((int)fromI->to, from-1);
        if(toI != styles.end())
            toI->from = qMax(to+1, (int)toI->from);
        styles.erase(fromI+1, toI);
        return styles.insert(toI, entry);
    }

    return styles.end();
};

void setFrom(int from, int to, int id) {
    auto i = iteratorFor(from, to, id);
    if(i == styles.end())
        return;

    i->from = from;
    i->to = to;
//    if(next != styles.end()) {
//        next->from = qMax(i->to + 1, (int)next->from);
//    }

}


void printList(const char* label, const StyleEntryList& l)
{
    qDebug() << "-----" << label;
    for(auto elem: l)
        qDebug() << elem;
}

void test(const char* testName, const StyleEntryList& a, const StyleEntryList& b){
    try {
        if(a.size() != b.size()) {
            std::exception e;
            throw "Size does not match";
        }
        auto ai = a.begin();
        auto ab = b.begin();
        for(;ai != a.end();++ai,++ab) {
            if(ai->id != ab->id) {
                qDebug() << ai->id << ab->id;
                throw "ID does not match";
            }
            if(ai->from != ab->from)
                throw "From does not match";
            if(ai->to != ab->to)
                throw "To does not match";
        }
        qDebug() << "[+] Test: "<< testName;

    } catch(const char* e) {
        qDebug() << e;
        printList("result  ", a);
        printList("shouldBe", b);
        qDebug() << "[-] Test: "<< testName;
    }
}

#define TEST(name, code) { \
    styles.clear(); \
    shouldBe.clear(); \
    code; \
    test(name, styles, shouldBe); \
}

int main(int argc, char *argv[])
{
    StyleEntryList shouldBe;

    TEST("Single", {
             setFrom(5, 10, 1);
             shouldBe << StyleEntry(5, 10, 1);
         });

    TEST("Append single", {
             setFrom(5, 10, 1);
             setFrom(13, 15, 2);
             shouldBe << StyleEntry(5, 10, 1);
             shouldBe << StyleEntry(13, 15, 2);
         });

    TEST("Prepend single", {
             setFrom(5, 10, 1);
             setFrom(1, 4, 2);
             shouldBe << StyleEntry(1, 4 , 2);
             shouldBe << StyleEntry(5, 10, 1);
         });

    TEST("Overlap single #1", {
             setFrom(5, 10, 1);
             setFrom(1, 12, 2);
             shouldBe << StyleEntry(1, 12, 2);
         });
    TEST("Overlap single #2", {
             setFrom(5, 10, 1);
             setFrom(5, 10, 2);
             shouldBe << StyleEntry(5, 10, 2);
             test("Overlap single #2", styles, shouldBe);
         });

    TEST("Overlap single #3", {
             setFrom(5, 10, 1);
             setFrom(6, 10, 2);
             shouldBe << StyleEntry(5, 5, 1);
             shouldBe << StyleEntry(6, 10, 2);
         });

    TEST("Overlap single #4", {
        setFrom(5, 10, 1);
        setFrom(5, 9, 2);
        shouldBe << StyleEntry(5, 9, 2);
        shouldBe << StyleEntry(10, 10, 1);
    });

    TEST("Overlap single #5", {
        setFrom(5, 10, 1);
        setFrom(6, 12, 2);
        shouldBe << StyleEntry(5, 5, 1);
        shouldBe << StyleEntry(6, 12, 2);
        test("Overlap single #5", styles, shouldBe);
    });

    TEST("Overlap single #6", {
        setFrom(5, 10, 1);
        setFrom(1, 9, 2);
        shouldBe << StyleEntry(1, 9, 2);
        shouldBe << StyleEntry(10, 10, 1);
        test("Overlap single #6", styles, shouldBe);
    });


    TEST("Inside single element", {
        setFrom(1, 10, 1);
        setFrom(2, 9, 2);
        shouldBe << StyleEntry(1, 1, 1);
        shouldBe << StyleEntry(2, 9, 2);
        shouldBe << StyleEntry(10, 10, 1);
    });

    TEST("Inside element multi", {
        setFrom(1, 10, 1);
        setFrom(11, 20, 3);
        setFrom(21, 30, 4);
        setFrom(2, 9, 2);
        shouldBe << StyleEntry(1, 1, 1);
        shouldBe << StyleEntry(2, 9, 2);
        shouldBe << StyleEntry(10, 10, 1);
        shouldBe << StyleEntry(11, 20, 3);
        shouldBe << StyleEntry(21, 30, 4);
    });


    TEST("Overlap multi #1", {
        setFrom(1, 10, 1);
        setFrom(11, 20, 2);
        setFrom(21, 30, 3);
        setFrom(4, 15, 4);
        shouldBe << StyleEntry(1, 3, 1);
        shouldBe << StyleEntry(4, 15, 4);
        shouldBe << StyleEntry(16, 20, 2);
        shouldBe << StyleEntry(21, 30, 3);
    });

    TEST("Overlap multi #2", {
        setFrom(1, 10, 1);
        setFrom(16, 20, 2);
        setFrom(21, 30, 3);
        setFrom(4, 15, 4);
        shouldBe << StyleEntry(1, 3, 1);
        shouldBe << StyleEntry(4, 15, 4);
        shouldBe << StyleEntry(16, 20, 2);
        shouldBe << StyleEntry(21, 30, 3);
    });

    TEST("Overlap multi #3", {
        setFrom(9, 10, 1);
        setFrom(16, 20, 2);
        setFrom(21, 30, 3);
        setFrom(4, 15, 4);
        shouldBe << StyleEntry(4, 15, 4);
        shouldBe << StyleEntry(16, 20, 2);
        shouldBe << StyleEntry(21, 30, 3);
    });


    TEST("Overlap multi #4", {
        setFrom(9, 10, 1);
        setFrom(16, 20, 2);
        setFrom(21, 30, 3);
        setFrom(17, 32, 4);
        shouldBe << StyleEntry(9, 10, 1);
        shouldBe << StyleEntry(16, 16, 2);
        shouldBe << StyleEntry(17, 32, 4);
    });


    return 0;
}
