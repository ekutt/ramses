//  -------------------------------------------------------------------------
//  Copyright (C) 2015 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "framework_common_gmock_header.h"
#include "gtest/gtest.h"
#include "Utils/BinaryFileOutputStream.h"
#include "Math3d/Matrix44f.h"

namespace ramses_internal
{
    class BinaryFileOutputStreamTest : public testing::Test
    {
    public:
        BinaryFileOutputStreamTest();
        ~BinaryFileOutputStreamTest();

        File m_file;
        String m_testString;
    };

    BinaryFileOutputStreamTest::BinaryFileOutputStreamTest()
        : m_file("TestFile.bin")
    {
        BinaryFileOutputStream outputStream(m_file);

        m_testString = "Dies ist ein Text";

        outputStream << 10 << 20.0f << m_testString;
    }

    BinaryFileOutputStreamTest::~BinaryFileOutputStreamTest()
    {
        m_file.close();
        m_file.remove();
    }

    TEST_F(BinaryFileOutputStreamTest, ReadSomeData)
    {
        EXPECT_TRUE(m_file.open(File::Mode::ReadOnlyBinary));

        {
            UInt    numBytes = 0;
            int32_t inValue  = 0;
            EXPECT_EQ(EStatus::Ok, m_file.read(reinterpret_cast<char*>(&inValue), sizeof(int32_t), numBytes));
            ASSERT_EQ(sizeof(int32_t), numBytes);
            EXPECT_EQ(10, inValue);
        }

        {
            UInt  numBytes = 0;
            float inValue  = 0.f;
            EXPECT_EQ(EStatus::Ok, m_file.read(reinterpret_cast<char*>(&inValue), sizeof(float), numBytes));
            EXPECT_EQ(sizeof(float), numBytes);
        }

        {
            UInt numBytes   = 0;
            char buffer[40] = {0};
            EXPECT_EQ(EStatus::Ok, m_file.read(buffer, sizeof(uint32_t), numBytes));
            EXPECT_EQ(EStatus::Ok, m_file.read(buffer, sizeof(char) * m_testString.size(), numBytes));
            buffer[m_testString.size()] = 0;
            EXPECT_EQ(sizeof(char) * m_testString.size(), numBytes);
            EXPECT_STREQ("Dies ist ein Text", buffer);
        }
    }

    TEST_F(BinaryFileOutputStreamTest, BadFileObj)
    {
        File file("some/non/existing/path");
        BinaryFileOutputStream outputStream(file);

        char data = '1';
        int size = 1;

        outputStream.write(&data, size);
    }

    TEST_F(BinaryFileOutputStreamTest, Matrix44f)
    {
        {
            ramses_internal::File f("testFile.ram");
            BinaryFileOutputStream s(f);

            Matrix44f m(1.0f, 5.0f, 9.0f, 13.0f, 2.0f, 6.0f, 10.0f, 14.0f, 3.0f, 7.0f, 11.0f, 15.0f, 4.0f, 8.0f, 12.0f,
                        16.0f);

            s << m;
        }

        {
            ramses_internal::File f("testFile.ram");
            EXPECT_TRUE(f.open(File::Mode::ReadOnlyBinary));

            Float data[16] = {0};
            UInt num;
            EXPECT_EQ(EStatus::Ok, f.read(reinterpret_cast<Char*>(&data[0]), sizeof(Float) * 16, num));
            for (UInt32 i = 0; i < 16; i++)
            {
                EXPECT_EQ(Float(i + 1), data[i]);
            }

            f.remove();
        }
    }
}
