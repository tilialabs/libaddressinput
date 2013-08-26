/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.i18n.addressinput.testing;

import junit.framework.TestCase;

import java.util.concurrent.TimeoutException;

/**
 * An extension of TestCase that provides delayTestFinish() and finishTest() methods that behave
 * like the corresponding methods in GWTTestCase for testing asynchronous code.
 */
public abstract class AsyncTestCase extends TestCase {
    /**
     * Tracks whether this test is completely done.
     */
    private boolean mTestIsFinished;

    /**
     * The system time in milliseconds when the test should time out.
     */
    private long mTestTimeoutMillis;

    /**
     * Puts the current test in asynchronous mode.
     *
     * @param timeoutMillis time to wait before failing the test for timing out
     */
    protected void delayTestFinish(int timeoutMillis) {
        mTestTimeoutMillis = System.currentTimeMillis() + timeoutMillis;
    }

    /**
     * Causes this test to succeed during asynchronous mode.
     */
    protected void finishTest() {
        mTestIsFinished = true;
        synchronized (this) {
            notify();
        }
    }

    protected void runTest() throws Throwable {
        mTestIsFinished = false;
        mTestTimeoutMillis = 0;
        super.runTest();

        if (mTestTimeoutMillis > 0) {
            long timeoutMillis = mTestTimeoutMillis - System.currentTimeMillis();
            if (timeoutMillis > 0) {
                synchronized (this) {
                    wait(timeoutMillis);
                }
            }
            if (!mTestIsFinished) {
                throw new TimeoutException("Waited " + timeoutMillis + " ms!");
            }
        }
    }
}
