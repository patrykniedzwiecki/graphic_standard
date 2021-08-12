/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "buffer_client_producer_remote_test.h"

#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "buffer_client_producer.h"
#include "buffer_consumer_listener.h"
#include "buffer_queue_producer.h"

namespace OHOS {
void BufferClientProducerRemoteTest::SetUpTestCase()
{
    pipe(pipeFd);

    pid = fork();
    if (pid < 0) {
        exit(1);
    }

    if (pid == 0) {
        sptr<BufferQueue> bq = new BufferQueue("test");
        ASSERT_NE(bq, nullptr);

        sptr<BufferQueueProducer> bqp = new BufferQueueProducer(bq);
        ASSERT_NE(bqp, nullptr);

        bq->Init();
        sptr<IBufferConsumerListener> listener = new BufferConsumerListener();
        bq->RegisterConsumerListener(listener);

        auto sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        sm->AddSystemAbility(systemAbilityID, bqp);

        char buf[10] = "start";
        write(pipeFd[1], buf, sizeof(buf));

        sleep(0);

        read(pipeFd[0], buf, sizeof(buf));

        sm->RemoveSystemAbility(systemAbilityID);

        exit(0);
    }

    char buf[10];
    read(pipeFd[0], buf, sizeof(buf));

    auto sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    robj = sm->GetSystemAbility(systemAbilityID);
    bp = iface_cast<IBufferProducer>(robj);
}

void BufferClientProducerRemoteTest::TearDownTestCase()
{
    bp = nullptr;
    robj = nullptr;

    char buf[10] = "over";
    write(pipeFd[1], buf, sizeof(buf));

    waitpid(pid, nullptr, 0);
}

namespace {
HWTEST_F(BufferClientProducerRemoteTest, IsProxy, testing::ext::TestSize.Level0)
{
    ASSERT_TRUE(robj->IsProxyObject());
}

HWTEST_F(BufferClientProducerRemoteTest, QueueSize, testing::ext::TestSize.Level0)
{
    ASSERT_EQ(bp->GetQueueSize(), (uint32_t)SURFACE_DEFAULT_QUEUE_SIZE);

    SurfaceError ret = bp->SetQueueSize(2);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->SetQueueSize(SURFACE_MAX_QUEUE_SIZE + 1);
    ASSERT_NE(ret, SURFACE_ERROR_OK);

    ASSERT_EQ(bp->GetQueueSize(), 2u);
}

HWTEST_F(BufferClientProducerRemoteTest, ReqCan, testing::ext::TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer;
    int32_t releaseFence;
    int32_t sequence;

    SurfaceError ret = bp->RequestBuffer(sequence, buffer, releaseFence, requestConfig, deletingBuffers);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
    ASSERT_NE(buffer, nullptr);

    ret = bp->CancelBuffer(sequence, bedata);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
}

HWTEST_F(BufferClientProducerRemoteTest, ReqCanCan, testing::ext::TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer;
    int32_t releaseFence;
    int32_t sequence;

    SurfaceError ret = bp->RequestBuffer(sequence, buffer, releaseFence, requestConfig, deletingBuffers);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
    ASSERT_EQ(buffer, nullptr);

    ret = bp->CancelBuffer(sequence, bedata);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->CancelBuffer(sequence, bedata);
    ASSERT_NE(ret, SURFACE_ERROR_OK);
}

HWTEST_F(BufferClientProducerRemoteTest, ReqReqReqCanCan, testing::ext::TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer1;
    sptr<SurfaceBuffer> buffer2;
    sptr<SurfaceBuffer> buffer3;
    int32_t releaseFence;
    int32_t sequence1;
    int32_t sequence2;
    int32_t sequence3;
    SurfaceError ret;

    ret = bp->RequestBuffer(sequence1, buffer1, releaseFence, requestConfig, deletingBuffers);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
    ASSERT_EQ(buffer1, nullptr);

    ret = bp->RequestBuffer(sequence2, buffer2, releaseFence, requestConfig, deletingBuffers);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
    ASSERT_NE(buffer2, nullptr);

    ret = bp->RequestBuffer(sequence3, buffer3, releaseFence, requestConfig, deletingBuffers);
    ASSERT_NE(ret, SURFACE_ERROR_OK);
    ASSERT_EQ(buffer3, nullptr);

    ret = bp->CancelBuffer(sequence1, bedata);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->CancelBuffer(sequence2, bedata);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->CancelBuffer(sequence3, bedata);
    ASSERT_NE(ret, SURFACE_ERROR_OK);
}

HWTEST_F(BufferClientProducerRemoteTest, SetQueueSizeDeleting, testing::ext::TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer;
    int32_t releaseFence;
    int32_t sequence;

    sptr<BufferClientProducer> bcp = static_cast<BufferClientProducer*>(bp.GetRefPtr());

    SurfaceError ret = bp->SetQueueSize(1);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->RequestBuffer(sequence, buffer, releaseFence, requestConfig, deletingBuffers);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
    ASSERT_EQ(buffer, nullptr);

    ret = bp->CancelBuffer(sequence, bedata);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->SetQueueSize(2);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
}

HWTEST_F(BufferClientProducerRemoteTest, ReqFlu, testing::ext::TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer;
    int32_t releaseFence;
    int32_t sequence;

    SurfaceError ret = bp->RequestBuffer(sequence, buffer, releaseFence, requestConfig, deletingBuffers);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->FlushBuffer(sequence, bedata, -1, flushConfig);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);
}

HWTEST_F(BufferClientProducerRemoteTest, ReqFluFlu, testing::ext::TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer;
    int32_t releaseFence;
    int32_t sequence;

    SurfaceError ret = bp->RequestBuffer(sequence, buffer, releaseFence, requestConfig, deletingBuffers);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->FlushBuffer(sequence, bedata, -1, flushConfig);
    ASSERT_EQ(ret, SURFACE_ERROR_OK);

    ret = bp->FlushBuffer(sequence, bedata, -1, flushConfig);
    ASSERT_NE(ret, SURFACE_ERROR_OK);
}
}
} // namespace OHOS
