#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "../util/thread.h"
#include "dxvk_include.h"

namespace dxvk {

  class DxvkGraphicsPipeline;
  class DxvkGraphicsPipelineInstance;

  /**
   * \brief Pipeline compiler
   *
   * Asynchronous pipeline compiler
   */
  class DxvkPipelineCompiler : public RcObject {

  public:

    DxvkPipelineCompiler();
    ~DxvkPipelineCompiler();

    /**
     * \brief Compiles a pipeline asynchronously
     *
     * This should be used to compile graphics
     * pipeline instances asynchronously.
     * \param [in] pipeline The pipeline object
     * \param [in] instance The pipeline instance
     */
    void queueCompilation(
      const Rc<DxvkGraphicsPipeline>&         pipeline,
      const Rc<DxvkGraphicsPipelineInstance>& instance,
      const DxvkRenderPassFormat&             format);

  private:

    struct PipelineEntry {
      Rc<DxvkGraphicsPipeline>         pipeline;
      Rc<DxvkGraphicsPipelineInstance> instance;
      DxvkRenderPassFormat             format;
    };

    std::atomic<bool>           m_compilerStop = { false };
    std::mutex                  m_compilerLock;
    std::condition_variable     m_compilerCond;
    std::queue<PipelineEntry>   m_compilerQueue;
    std::vector<dxvk::thread>   m_compilerThreads;

    void runCompilerThread();

  };

}
